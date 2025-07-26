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

#define CGI_ERROR(state) \
    do { \
        DEBUG("CGI_ERROR at " << __FILE__ << ":" << __LINE__ << " (" << __func__ << "), setting _innerStatusCode to " << (state)); \
        _innerStatusCode = state;\
    } while(0)

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

CGIResponse::CGIResponse(Server &server, SocketFD &socketFD, Client *client, Request *request) : _server(server),
    _cgiOutputFD(), _cgiInputFD(),
    _environmentVariables(),
    _timerId(-1), _processId(-1),
    _chunkedRequestBodyRead(false), _hasSentFinalChunk(false), _isBrokenBeyondRepair(false),
    _transferMode(CGIResponseTransferMode::FullBuffer),
    _innerStatusCode(HttpStatusCode::OK), socketFD(socketFD), client(client) {
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
    _environmentVariables["REQUEST_METHOD"] = methodToStr(client->request.metadata.getMethod());
    _environmentVariables["PATH_INFO"] = _request->metadata.getRawUrl();
    _environmentVariables["PATH_TRANSLATED"] = std::string(parsedUrl.scriptPath + parsedUrl.pathInfo);
    _environmentVariables["SCRIPT_FILENAME"] = parsedUrl.scriptPath;
    _environmentVariables["SCRIPT_NAME"] = Path(parsedUrl.scriptPath).getFilename();
    _environmentVariables["QUERY_STRING"] = std::string(parsedUrl.query);
    _environmentVariables["REMOTE_ADDR"] = client->getClientIP();
    _environmentVariables["REMOTE_PORT"] = client->getClientPort();
    _environmentVariables["SERVER_ADDR"] = client->getServer().getServerAddress();
    _environmentVariables["SERVER_PORT"] = std::to_string(config.port.getPort());
    _environmentVariables["REDIRECT_STATUS"] = "200";

    DEBUG("Upload store: " << route.uploadStore.getUploadDir().str());
    if (route.uploadStore.isSet()) {
        DEBUG("Setting WEBSERV_UPLOAD_STORE environment variable for CGI: " << route.uploadStore.getUploadDir().str());
        Path uploadStorePath = Path(serverExecutablePath).append(route.uploadStore.getUploadDir().str());
        DEBUG("Upload store path: " << uploadStorePath.str());
        _environmentVariables["WEBSERV_UPLOAD_STORE"] = uploadStorePath.str();
    }

    if (client->request.session && !client->request.session->sessionId.empty())
        _environmentVariables["HTTP_SESSION_FILE"] = client->request.session->absoluteFilePath;

    const std::string contentHeader = client->request.headers.getHeader(HeaderKey::ContentType, "");
    if (!contentHeader.empty())
        _environmentVariables["CONTENT_TYPE"] = contentHeader;

    const std::string contentLength = client->request.headers.getHeader(HeaderKey::ContentLength, "");
    if (!contentLength.empty())
        _environmentVariables["CONTENT_LENGTH"] = contentLength;

    for (const auto &[headerKey, headerValue] : client->request.headers.getHeaders()) {
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

void CGIResponse::start(const ServerConfig &config, const LocationRule &route, const Path &serverExecutablePath) {
    DEBUG("Starting CGIResponse for client: " << client);
    const ParsedUrl parsedUrl = parseUrl(route, client->request.metadata);

    DEBUG("Parsed URL: scriptPath=" << parsedUrl.scriptPath
          << ", pathInfo=" << parsedUrl.pathInfo
          << ", query=" << parsedUrl.query
          << ", isValid=" << parsedUrl.isValid);

    if (!parsedUrl.isValid) {
        DEBUG("Unknown URL for CGI processing: " << client->request.metadata.getRawUrl());
        CGI_ERROR(HttpStatusCode::NotFound);
        return;
    }

    if (!(route.cgiExtension.isCGI(Path(parsedUrl.scriptPath)) || route.cgi.isSet())) {
        ERROR("The requested path is not a CGI script: " << parsedUrl.scriptPath);
        CGI_ERROR(HttpStatusCode::Forbidden);
        return;
    }

    _setupEnvironmentVariables(config, route, parsedUrl, serverExecutablePath);

    int cin[2], cout[2];
    if (pipe(cin) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
        CGI_ERROR(HttpStatusCode::InternalServerError);
        return;
    }

    if (pipe(cout) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
        close(cin[0]);
        close(cin[1]);
        CGI_ERROR(HttpStatusCode::InternalServerError);
        return;
    }

    _processId = fork();
    if (_processId < 0) {
        ERROR("Failed to fork CGI process for client: " << &client);
        close(cin[0]);
        close(cin[1]);
        close(cout[0]);
        close(cout[1]);
        CGI_ERROR(HttpStatusCode::InternalServerError);
        return;
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
            CGI_ERROR(HttpStatusCode::InternalServerError);
            return;
        }

        if (_cgiInputFD.connectToEpoll(_server.getEpollFd(), DEFAULT_EPOLLOUT_EVENTS) == -1 ||
            _cgiOutputFD.connectToEpoll(_server.getEpollFd(), DEFAULT_EPOLLIN_EVENTS) == -1) {
            ERROR("Failed to connect CGI pipes to epoll: " << strerror(errno));
            _closeToCGIProcessFd();
            _closeFromCGIProcessFd();
            CGI_ERROR(HttpStatusCode::InternalServerError);
            return;
        }

        _timerId = _server.getTimer().addEvent(std::chrono::milliseconds(static_cast<int>(route.cgiTimeout.timeout.getSeconds() * 1000.0)), [this]() {
            _handleTimeout();
        });

        _server.trackCallbackFD(_cgiInputFD, [this](WritableFD &fd, short revents) {
            (void) fd;
            DEBUG("CGIResponse read callback, fd: " << fd.get() << ", revents: " << revents);
            if (revents & EPOLLOUT) _cgiInputFD.setWriterFDState(FDState::Ready);
            else _cgiInputFD.setWriterFDState(FDState::Awaiting);
        });
        _server.trackCallbackFD(_cgiOutputFD, [this](ReadableFD &fd, short revents) {
            (void) fd;
            DEBUG("CGIResponse write callback, fd: " << fd.get() << ", revents: " << revents);
            if (fd.getReaderFDState() == FDState::Ready) fd.read();
            if (revents & EPOLLHUP) {
                DEBUG("CGI process output pipe closed, setting state to Closed");
                _closeFromCGIProcessFd();
            }
        });

        _server.trackCGIResponse(this);
        DEBUG("CGI process started with PID: " << _processId << ", cgiInputFD: " << _cgiInputFD.get() << ", cgiOutputFD: " << _cgiOutputFD.get());
    }
}

ssize_t CGIResponse::_sendRequestBodyToCGIProcess() {
    ssize_t bytesWritten = 0;

    switch (client->request.receivingBodyMode) {
        case ReceivingBodyMode::Chunked: {
            while (true) {
                FDReader::HTTPChunk chunk = socketFD.extractHTTPChunkFromReadBuffer();
                if (chunk.size == FDReader::HTTPChunk::noChunk) return (bytesWritten);
                if (chunk.size == 0) {
                    _chunkedRequestBodyRead = true;
                    return (0);
                }

                ssize_t written = _cgiInputFD.writeAsString(chunk.data);
                if (written < 0) {
                    ERROR("Failed to write chunk to CGI process");
                    return (0);
                }
                bytesWritten += written;
                DEBUG("Sent chunk to CGI process: " << chunk.data);
            }
        }

        default: {
            bytesWritten = _cgiInputFD.writeAsString(socketFD.extractFullBuffer());
            if (bytesWritten < 0) {
                ERROR("Failed to write request body to CGI process");
                CGI_ERROR(HttpStatusCode::InternalServerError);
            }
            return (bytesWritten);
        }
    }
}

bool CGIResponse::_fetchCGIHeadersFromProcess() {
    std::string cgiHeaderString = _cgiOutputFD.extractHeadersFromReadBuffer();
    DEBUG_ESC("Headers gotten: " << cgiHeaderString);
    // if (cgiHeaderString.empty()) return (false);

    std::istringstream cgiHeaderStream(cgiHeaderString);
    Headers cgiHeaders(cgiHeaderStream);
    headers.merge(cgiHeaders);

    setStatusCode(HttpStatusCode::OK);
	
    // try {
    //     int code = std::stoi(headers.getAndRemoveHeader(HeaderKey::Status, "l"));
    //     if (code < 100 || code > 599) {
    //         CGI_ERROR(HttpStatusCode::InternalServerError);
    //         return (false);
    //     }
    //     setStatusCode(static_cast<HttpStatusCode>(code));
    // } catch (...) {
    //     ERROR("Failed to parse CGI response status code");
	// 	setStatusCode(HttpStatusCode::OK);
    //     // CGI_ERROR(HttpStatusCode::InternalServerError);
    // }

    if (_transferMode == CGIResponseTransferMode::FullBuffer) {
        headers.replace(HeaderKey::ContentLength, std::to_string(_cgiOutputFD.getReadBufferSize()));
        headers.remove(HeaderKey::TransferEncoding);
    } else {
        headers.replace(HeaderKey::ContentDisposition, "attachment; filename=\"response\"");
        headers.replace(HeaderKey::ContentType, "application/octet-stream");
        headers.replace(HeaderKey::TransferEncoding, "chunked");
        headers.remove(HeaderKey::ContentLength);
    }

    return (true);
}

void CGIResponse::_sendCGIResponse() {
    if (socketFD.getWriterFDState() != FDState::Ready) {
        DEBUG("Socket is not ready for writing, waiting for it to be ready");
        return;
    }

    if (!headersBeenSent() && !_fetchCGIHeadersFromProcess()) {
        ERROR("Failed to fetch CGI headers");
        CGI_ERROR(HttpStatusCode::InternalServerError);
        return;
    }

    if (!headersBeenSent())
        sendHeaders(socketFD);

    switch (_transferMode) {
        case CGIResponseTransferMode::FullBuffer: {
            ssize_t bytesWritten = socketFD.writeAsString(_cgiOutputFD.extractFullBuffer());
            if (bytesWritten < 0) {
                ERROR("Failed to write full CGI response to socket");
                CGI_ERROR(HttpStatusCode::InternalServerError);
            }
            return;
        }

        case CGIResponseTransferMode::Chunked: {
            std::string fullBuffer = _cgiOutputFD.extractChunkFromReadBuffer(DEFAULT_CHUNK_SIZE);
            DEBUG("Sending chunked CGI response to socket, size: " << fullBuffer.size());
            ssize_t bytesWritten = socketFD.writeAsChunk(fullBuffer);
            if (bytesWritten < 0) {
                ERROR("Failed to write chunked CGI response to socket");
                ERROR(errno << ": " << strerror(errno));
                // _isBrokenBeyondRepair = true;
            }
            return;
        }
    }
}

void CGIResponse::tick() {
    DEBUG("CGIResponse tick for client: " << client);
    if (_cgiInputFD.isValidFd() && _cgiInputFD.getWriterFDState() == FDState::Ready) {
        DEBUG("CGIResponse input pipe is ready for writing, fd: " << _cgiInputFD.get());
        DEBUG("read buffer size: " << socketFD.getReadBufferSize());
        if (socketFD.getReadBufferSize() > 0)
            _sendRequestBodyToCGIProcess();
        else if (client->isFullRequestBodyReceived(socketFD))
            _closeToCGIProcessFd();
    }

    DEBUG("CGIResponse output pipe state: " << static_cast<int>(_cgiOutputFD.getReaderFDState()) << ", fd: " << _cgiOutputFD.get());
    if (_cgiOutputFD.getReadBufferSize() > 0) {
        if (!headersBeenSent() && _cgiOutputFD.wouldReadExceedMaxBufferSize()) {
            if (_transferMode == CGIResponseTransferMode::FullBuffer) {
                DEBUG("CGI output is too large for full buffer mode, switching to chunked mode");
                _transferMode = CGIResponseTransferMode::Chunked;
            }
        }

        if (_transferMode == CGIResponseTransferMode::Chunked || !_cgiOutputFD.isValidFd())
            _sendCGIResponse();
    }

    else if ((!_cgiOutputFD.isValidFd() || _cgiOutputFD.getReaderFDState() == FDState::Closed) && _cgiOutputFD.getReadBufferSize() == 0) {
        if (_transferMode == CGIResponseTransferMode::Chunked && !_hasSentFinalChunk) {
            if (socketFD.getWriterFDState() != FDState::Ready) {
                DEBUG("Waiting for socket to be ready for writing, fd: " << socketFD.get() << ", state: " << static_cast<int>(socketFD.getWriterFDState()));
                return;
            }
            DEBUG("Sending final chunk to socket");
            ssize_t bytesWritten = socketFD.writeAsChunk("");
            if (bytesWritten < 0) {
                ERROR("Failed to write final chunk to socket");
                CGI_ERROR(HttpStatusCode::InternalServerError);
                return;
            }
            _hasSentFinalChunk = true;
        }

        int exitStatus = 0;
        if (_processId != -1 && waitpid(_processId, &exitStatus, WNOHANG) <= 0) {
            DEBUG("CGI process is still running, waiting for it to finish");
            return;
        }

        _processId = -1;
        exitStatus = WEXITSTATUS(exitStatus);
        DEBUG("CGI process exited with status: " << exitStatus);

        if (exitStatus != 0) {
            DEBUG("CGI process exited with non-zero status: " << exitStatus);
            if (_transferMode == CGIResponseTransferMode::Chunked)
                _isBrokenBeyondRepair = true;
            else
                CGI_ERROR(HttpStatusCode::InternalServerError);
            return;
        }
    }
}

void CGIResponse::handleRequestBody(SocketFD &fd, const Request &request) {
    (void) fd;
    (void) request;
}

void CGIResponse::handleSocketWriteTick(SocketFD &fd) {
    (void) fd;
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
    _server.untrackCGIResponse(this);
}

void CGIResponse::_handleTimeout() {
    DEBUG("CGIResponse timeout handler called for client: " << client);
    CGI_ERROR(HttpStatusCode::GatewayTimeout);
}

CGIResponse::~CGIResponse() {
    DEBUG("CGIResponse destructor called for client: " << client);
    terminateResponse();
}

bool CGIResponse::isFullResponseSent() const {
    return (_cgiOutputFD.getReaderFDState() == FDState::Closed && _cgiOutputFD.getReadBufferSize() == 0 &&
        !didResponseCreationFail() && _processId == -1);
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
