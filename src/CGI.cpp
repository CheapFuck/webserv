#include "config/types/customTypes.hpp"
#include "config/rules/rules.hpp"
#include "response.hpp"
#include "timer.hpp"
#include "print.hpp"

#include <sys/wait.h>
#include <filesystem>
#include <signal.h>
#include <cstring>
#include <chrono>

/// @brief Parses the given URL and extracts the script path, path info, and query string.
/// @param url The URL to parse, which may include a query string.
/// @param route The location rule to use for parsing the URL.
/// @return A ParsedUrl struct containing the extracted information.
static ParsedUrl parseUrl(const LocationRule &route, RequestLine &requestLine) {
    DEBUG("Parsing URL: " << requestLine.getRawUrl());

    Path originalUrl = Path::createFromUrl(requestLine.getRawUrl(), route);
    Path tmp = originalUrl;
    DEBUG("Initial path for CGI script search: " << tmp.str());
    while (!tmp.str().empty()) {
        DEBUG("Checking for CGI script: " << tmp.str());
        if (std::filesystem::exists(tmp.str())) {
            if (std::filesystem::is_regular_file(tmp.str())) {
                Path CGIPath = Path(requestLine.getServerAbsolutePath()).append(tmp.str());
                DEBUG("Found CGI script: " << tmp.str());
                return (ParsedUrl{
                    .scriptPath = CGIPath.str(),
                    .pathInfo = originalUrl.str().substr(tmp.str().length()),
                    .query = (requestLine.getRawUrl().find('?') != std::string::npos ? requestLine.getRawUrl().substr(requestLine.getRawUrl().find('?') + 1) : ""),
                    .isValid = true
                });
            } else if (route.index.isSet() && std::filesystem::is_directory(tmp.str())) {
                DEBUG("Found directory: " << tmp.str());
                for (const auto &indexFile : route.index.getIndexFiles()) {
                    Path indexPath = tmp;
                    indexPath.append(indexFile);
                    DEBUG("Checking for index file: " << indexPath.str());
                    if (std::filesystem::exists(indexPath.str()) && std::filesystem::is_regular_file(indexPath.str())) {
                        DEBUG("Found index file: " << indexPath.str());
                        return (ParsedUrl{
                            .scriptPath = indexPath.str(),
                            .pathInfo = originalUrl.str().substr(tmp.str().length()),
                            .query = (requestLine.getRawUrl().find('?') != std::string::npos ? requestLine.getRawUrl().substr(requestLine.getRawUrl().find('?') + 1) : ""),
                            .isValid = true
                        });
                    }
                }
            } else {
                break ;
            }
        }
        tmp.pop();
    }

    return (ParsedUrl{
        .scriptPath = "",
        .pathInfo = "",
        .query = "",
        .isValid = false
    });
}

CGIResponse::CGIResponse(Client *client, Server &server, SocketFD &socketFD, Request *request) :
    Response(client),
    _server(server),
    _cgiOutputFD(), _cgiInputFD(),
    _environmentVariables(),
    _pipeWriter(),
    _sendBytesTracker(0), _responseLength(0),
    _timerId(-1), _processId(-1),
    _chunkedRequestBodyRead(false), _hasSentFinalChunk(false), _isBrokenBeyondRepair(false),
    _transferMode(CGIResponseTransferMode::Unknown),
    _innerStatusCode(HttpStatusCode::OK), socketFD(socketFD) {
    DEBUG("CGIResponse created for client: " << client);
	_request = request;
}

bool CGIResponse::didResponseCreationFail() const {
    return (_innerStatusCode != HttpStatusCode::OK);
}

HttpStatusCode CGIResponse::getFailedResponseStatusCode() const {
    return (_innerStatusCode);
}

/// @brief Sets up the environment variables for the CGI process.
/// @details https://www6.uniovi.es/~antonio/ncsa_httpd/cgi/env.html
/// @param config The server configuration
/// @param route The location rule
void CGIResponse::_setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl, const Path &serverExecutablePath) {
    _environmentVariables.clear();
    _environmentVariables["SERVER_SOFTWARE"] = "webserv/1.0";
    _environmentVariables["SERVER_NAME"] = std::string(config.serverName.getServerName());
    _environmentVariables["GATEWAY_INTERFACE"] = "CGI/1.1";

    _environmentVariables["SERVER_PROTOCOL"] = Response::protocol + std::string("/") + Response::tlsVersion;
    _environmentVariables["SERVER_PORT"] = std::to_string(config.port.getPort());
    _environmentVariables["REQUEST_METHOD"] = methodToStr(_client->request.metadata.getMethod());
    _environmentVariables["PATH_INFO"] = _request->metadata.getRawUrl();
    _environmentVariables["PATH_TRANSLATED"] = std::string(parsedUrl.scriptPath + parsedUrl.pathInfo);
    _environmentVariables["SCRIPT_FILENAME"] = parsedUrl.scriptPath;
    _environmentVariables["SCRIPT_NAME"] = Path(parsedUrl.scriptPath).getFilename();
    _environmentVariables["QUERY_STRING"] = std::string(parsedUrl.query);
    _environmentVariables["REMOTE_ADDR"] = _client->getClientIP();
    _environmentVariables["REMOTE_PORT"] = _client->getClientPort();
    _environmentVariables["SERVER_ADDR"] = _client->getServer().getServerAddress();
    _environmentVariables["SERVER_PORT"] = std::to_string(config.port.getPort());
    _environmentVariables["REDIRECT_STATUS"] = "200";

    DEBUG("Upload store: " << route.uploadStore.getUploadDir().str());
    if (route.uploadStore.isSet()) {
        DEBUG("Setting WEBSERV_UPLOAD_STORE environment variable for CGI: " << route.uploadStore.getUploadDir().str());
        Path uploadStorePath = Path(serverExecutablePath).append(route.uploadStore.getUploadDir().str());
        DEBUG("Upload store path: " << uploadStorePath.str());
        _environmentVariables["WEBSERV_UPLOAD_STORE"] = uploadStorePath.str();
    }

    if (_client->request.session && !_client->request.session->sessionId.empty())
        _environmentVariables["HTTP_SESSION_FILE"] = _client->request.session->absoluteFilePath;

    const std::string contentHeader = _client->request.headers.getHeader(HeaderKey::ContentType, "");
    if (!contentHeader.empty())
        _environmentVariables["CONTENT_TYPE"] = contentHeader;

    const std::string contentLength = _client->request.headers.getHeader(HeaderKey::ContentLength, "");
    if (!contentLength.empty())
        _environmentVariables["CONTENT_LENGTH"] = contentLength;

    for (const auto &[headerKey, headerValue] : _client->request.headers.getHeaders()) {
        std::string envVarKey = "HTTP_" + headerKey;
        std::replace(envVarKey.begin(), envVarKey.end(), '-', '_');
        std::transform(envVarKey.begin(), envVarKey.end(), envVarKey.begin(), ::toupper);
        _environmentVariables[envVarKey] = headerValue;
    }
}

void CGIResponse::_createEnvironmentArray(std::vector<char*> &envPtrs, std::vector<std::string> &strBuff) const {
    strBuff.clear();
    envPtrs.clear();

    strBuff.reserve(_environmentVariables.size());
    envPtrs.reserve(_environmentVariables.size() + 1);

for (const auto &env : _environmentVariables) {
        strBuff.push_back(env.first + "=" + env.second);
        envPtrs.push_back(const_cast<char*>(strBuff.back().c_str()));
    }
    envPtrs.push_back(nullptr);
}

bool CGIResponse::start(const ServerConfig &config, const LocationRule &route, const Path &serverExecutablePath) {
    DEBUG("Starting CGIResponse for client: " << _client);
    const ParsedUrl parsedUrl = parseUrl(route, _client->request.metadata);

    DEBUG("Parsed URL: scriptPath=" << parsedUrl.scriptPath
          << ", pathInfo=" << parsedUrl.pathInfo
          << ", query=" << parsedUrl.query
          << ", isValid=" << parsedUrl.isValid);

    _setupEnvironmentVariables(config, route, parsedUrl, serverExecutablePath);

    int cin[2], cout[2];
    if (pipe(cin) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
        return (false);
    }

    if (pipe(cout) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
        close(cin[0]);
        close(cin[1]);
        return (false);
    }

    _processId = fork();
    if (_processId < 0) {
        ERROR("Failed to fork CGI process for client: " << &_client);
        close(cin[0]);
        close(cin[1]);
        close(cout[0]);
        close(cout[1]);
        return (false);
    }

    if (_processId == 0) {
        // Child process

        Path scriptDir = Path(parsedUrl.scriptPath).pop();
        DEBUG("CGI script directory: " << scriptDir.str());
        if (chdir(scriptDir.str().c_str()) == -1) {
            ERROR("Failed to change directory to CGI script path: " << parsedUrl.scriptPath << ", errno: " << errno << " (" << strerror(errno) << ")");
            close(cin[0]);
            close(cin[1]);
            close(cout[0]);
            close(cout[1]);
            exit(EXIT_FAILURE);
        }

        FD childStdin = ReadableFD::pipe(cin);
        FD childStdout = WritableFD::pipe(cout);
        if (dup2(childStdin.get(), STDIN_FILENO) == -1 || dup2(childStdout.get(), STDOUT_FILENO) == -1) {
            childStdin.close();
            childStdout.close();
            ERROR("Failed to redirect stdin/stdout for CGI process: " << strerror(errno));
            exit(EXIT_FAILURE);
        }

        childStdin.close();
        childStdout.close();

        std::string scriptName = Path(parsedUrl.scriptPath).getFilename();
        char * const argv[] = {const_cast<char *>(scriptName.c_str()), nullptr};

        // Debug argv
        std::vector<char*> envPtrs;
        std::vector<std::string> strBuff;
        _createEnvironmentArray(envPtrs, strBuff);

        if (execve(scriptName.c_str(), argv, envPtrs.data()) == -1) {
            ERROR("Failed to execute CGI script: " << parsedUrl.scriptPath << ", errno: " << errno << " (" << strerror(errno) << ")");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        _cgiOutputFD = ReadableFD::pipe(cout);
        _cgiInputFD = WritableFD::pipe(cin);

        if (_cgiInputFD.setNonBlocking() == -1 || _cgiOutputFD  .setNonBlocking() == -1) {
            ERROR("Failed to set non-blocking mode for CGI pipes: " << strerror(errno));
            _closeToCGIProcessFd();
            _closeFromCGIProcessFd();
            return (false);
        }

        if (_cgiInputFD.connectToEpoll(_server.getEpollFd(), DEFAULT_EPOLLOUT_EVENTS) == -1 ||
            _cgiOutputFD.connectToEpoll(_server.getEpollFd(), DEFAULT_EPOLLIN_EVENTS) == -1) {
            ERROR("Failed to connect CGI pipes to epoll: " << strerror(errno));
            _closeToCGIProcessFd();
            _closeFromCGIProcessFd();
            return (false);
        }

        _timerId = _server.getTimer().addEvent(std::chrono::milliseconds(static_cast<int>(route.cgiTimeout.timeout.getSeconds() * 1000.0)), [this]() {
            _handleTimeout();
        });

        _server.trackCallbackFD(_cgiInputFD, [this](WritableFD &fd, short revents) {
            _handleCGIInputPipeEvent(fd, revents);
        });
        _server.trackCallbackFD(_cgiOutputFD, [this](ReadableFD &fd, short revents) {
            _handleCGIOutputPipeEvent(fd, revents);
        });

        // _server.trackCGIResponse(this);
        DEBUG("CGI process started with PID: " << _processId << ", cgiInputFD: " << _cgiInputFD.get() << ", cgiOutputFD: " << _cgiOutputFD.get());
    }

    return (true);
}

void CGIResponse::_handleCGIInputPipeEvent(WritableFD &fd, short revents) {
    if (revents & EPOLLOUT) {
        DEBUG("CGI input pipe is ready for writing, fd: " << fd.get());

        if (!_pipeWriter.isEmpty()) {
            _pipeWriter.tick(_cgiInputFD);
        } else {
            switch (_client->request.receivingBodyMode) {
                case ReceivingBodyMode::Chunked: {
                    FDReader::HTTPChunk chunk = socketFD.extractHTTPChunkFromReadBuffer();
                    if (chunk.size == FDReader::HTTPChunk::noChunk) break ;
                    if (chunk.size == 0) {
                        _chunkedRequestBodyRead = true;
                        DEBUG("Chunked request body read complete, no more data to send");
                        break ;
                    }

                    _pipeWriter.sendBodyAsString(chunk.data, fd);
                    DEBUG("Sent chunked request body to CGI process, size: " << chunk.size);
                    break ;
                }

                case ReceivingBodyMode::ContentLength:
                case ReceivingBodyMode::NotSet: {
                    _pipeWriter.sendBodyAsString(socketFD, fd);
                    DEBUG("Sent request body to CGI process, size: " << socketFD.getReadBufferSize());
                    break ;
                }
            }
        }
    }

    if (_client->route && _client->route->maxBodySize.isSet()
        && _client->route->maxBodySize.getMaxBodySize().get() < static_cast<size_t>(socketFD.getTotalBodyBytes())) {
        DEBUG("Request body exceeds max body size, using error response");
        _client->switchResponseToErrorResponse(HttpStatusCode::PayloadTooLarge, socketFD);
        return ;
    }
    
    if (revents & (EPOLLHUP | EPOLLERR)) {
        DEBUG("CGI input pipe closed");
        _closeToCGIProcessFd();
    }

    if (_client->isFullRequestBodyReceived(socketFD) && socketFD.getReadBufferSize() == 0) {
        DEBUG("Full request body received, closing CGI input pipe");
        _closeToCGIProcessFd();
    }
}

void CGIResponse::_handleCGIOutputPipeEvent(ReadableFD &fd, short revents) {
    if (revents & EPOLLIN) {
        DEBUG("CGI output pipe is ready for reading, fd: " << fd.get());

        if (fd.wouldReadExceedMaxBufferSize()) {
            if (_transferMode == CGIResponseTransferMode::Unknown) {
                if (_prepareCGIResponse() != HttpStatusCode::OK) return ;
            }
        } else {
            fd.read();
        }
    }

    if (revents & (EPOLLHUP | EPOLLERR)) {
        DEBUG("CGI output pipe closed");
        _closeFromCGIProcessFd();

        if (_transferMode == CGIResponseTransferMode::Unknown) {
            if (_prepareCGIResponse() != HttpStatusCode::OK) return ;
        }
    }

    if (_transferMode != CGIResponseTransferMode::Unknown
        && !_client->setEpollWriteNotification(socketFD))
        return ;
}

HttpStatusCode CGIResponse::_prepareCGIResponse() {
    std::string cgiHeaderString = _cgiOutputFD.extractHeadersFromReadBuffer();
    DEBUG_ESC("Headers gotten: " << cgiHeaderString);
    if (cgiHeaderString.empty()) {
        _client->switchResponseToErrorResponse(HttpStatusCode::InternalServerError, socketFD);
        return (HttpStatusCode::InternalServerError);
    }

    std::istringstream cgiHeaderStream(cgiHeaderString);
    Headers cgiHeaders(cgiHeaderStream);
    headers.merge(cgiHeaders);

    try {
        int code = std::stoi(headers.getAndRemoveHeader(HeaderKey::Status, "l"));
        if (code < 100 || code > 599) {
            return (HttpStatusCode::InternalServerError);
        }
        setStatusCode(static_cast<HttpStatusCode>(code));
        if (code / 100 != 2)
            return (static_cast<HttpStatusCode>(code));
    } catch (...) {
        return (HttpStatusCode::InternalServerError);
    }

    if (headers.getHeader(HeaderKey::ContentLength, "").empty()
        || headers.getHeader(HeaderKey::TransferEncoding, "") == "chunked") {
        headers.remove(HeaderKey::ContentLength);
        headers.replace(HeaderKey::TransferEncoding, "chunked");
        _transferMode = CGIResponseTransferMode::Chunked;
    }

    else {
        _transferMode = CGIResponseTransferMode::FullBuffer;
    }

    return (HttpStatusCode::OK);
}

void CGIResponse::_sendCGIResponse() {
    return ;
}

void CGIResponse::tick() {
    return ;
}

void CGIResponse::handleRequestBody(SocketFD &fd, const Request &request) {
    (void) fd;
    (void) request;
}

void CGIResponse::handleSocketWriteTick(SocketFD &fd) {
    if (_transferMode == CGIResponseTransferMode::Unknown)
        return ;
    DEBUG("CGIResponse handleSocketWriteTick for client: " << _client << ", fd: " << fd.get());

    // if (_processId != -1) {
    //     int status;
    //     if (waitpid(_processId, &status, WNOHANG) == -1) {
    //         DEBUG("CGI process not yet finished, waiting for it to complete");
    //         return ;
    //     }
    //     PRINT("Return thingy code" << WEXITSTATUS(status));

    //     if (WEXITSTATUS(status) != 0) {
    //         ERROR("CGI process exited with error, status: " << WEXITSTATUS(status));
    //         _client->switchResponseToErrorResponse(HttpStatusCode::InternalServerError, socketFD);
    //         return ;
    //     }

    //     _processId = -1;
    // }

    if (!headersBeenSent())
        return (sendHeaders(fd));

    switch (_transferMode) {
        case CGIResponseTransferMode::Chunked: {
			// if (_cgiOutputFD.getReadBufferSize() < 1024 * 1024 && _cgiOutputFD.getReaderFDState() != FDState::Closed) break ;
            if (_bodyWriter.sendBodyAsHTTPChunk(_cgiOutputFD, fd) != 0)
                return ;
            break ;
        }

        case CGIResponseTransferMode::FullBuffer: {
			// if (_cgiOutputFD.getReadBufferSize() < 1024 * 1024 && _cgiOutputFD.getReaderFDState() != FDState::Closed) break ;
            if (_bodyWriter.sendBodyAsString(_cgiOutputFD, fd) != 0)
                return ;
            break ;
        }

        default: {
            ERROR("Unknown transfer mode: " << static_cast<int>(_transferMode));
            break ;
        }
    }

    if (!_bodyWriter.isEmpty())
        return (_bodyWriter.tick(fd), void());

    if (_cgiOutputFD.getReaderFDState() == FDState::Closed && _cgiOutputFD.getReadBufferSize() == 0) {
        DEBUG("CGI output pipe closed, sending final chunk");
        if (_transferMode == CGIResponseTransferMode::Chunked && !_hasSentFinalChunk) {
            sendBodyAsChunk(fd, "");
            DEBUG("Amount of bytes sent to CGI process: " << _pipeWriter.amountOfBytesWritten);
            _hasSentFinalChunk = true;
            return ;
        }
    }
}

void CGIResponse::terminateResponse() {
    _closeToCGIProcessFd();
    _closeFromCGIProcessFd();
    if (_timerId != -1) {
        _server.getTimer().deleteEvent(_timerId);
        _timerId = -1;
    }
    if (_processId != -1) {
        kill(_processId, SIGKILL);
        _processId = -1;
    }
}

void CGIResponse::_handleTimeout() {
    DEBUG("CGIResponse timeout handler called for client: " << _client);
    _client->switchResponseToErrorResponse(HttpStatusCode::RequestTimeout, socketFD);
}

CGIResponse::~CGIResponse() {
    DEBUG("CGIResponse destructor called for client: " << _client);
    terminateResponse();
}

bool CGIResponse::isFullResponseSent() const {
    return (_cgiOutputFD.getReaderFDState() == FDState::Closed && _cgiOutputFD.getReadBufferSize() == 0 &&
        !didResponseCreationFail() && _bodyWriter.isEmpty() && ((_transferMode == CGIResponseTransferMode::Chunked && _hasSentFinalChunk) || (_transferMode == CGIResponseTransferMode::FullBuffer && _sendBytesTracker >= _responseLength)));
}

void CGIResponse::_closeToCGIProcessFd() {
    if (_cgiInputFD.isValidFd()) {
        _server.untrackCallbackFD(_cgiInputFD);
        _cgiInputFD.setWriterFDState(FDState::Closed);
        _cgiInputFD.close();
    }
}

void CGIResponse::_closeFromCGIProcessFd() {
    if (_cgiOutputFD.isValidFd()) {
        _server.untrackCallbackFD(_cgiOutputFD);
        _cgiOutputFD.setReaderFDState(FDState::Closed);
        _cgiOutputFD.close();
    }
}

bool CGIResponse::isBrokenBeyondRepair() const {
    return (_isBrokenBeyondRepair);
}
