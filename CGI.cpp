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

CGIResponse::CGIResponse(Server &server, SocketFD &socketFD, std::shared_ptr<Client> client) : _server(server), _client(client), _socketFD(socketFD),
    _cgiOutputFD(), _cgiInputFD(),
    _timerId(-1), _processTimerId(-1) {
    DEBUG("CGIResponse created for client: " << _client.get());
}

CGIResponse::CGIResponse(const CGIResponse &other) :
      _server(other._server),
      _client(other._client),
      _socketFD(other._socketFD),
      _cgiOutputFD(other._cgiOutputFD),
      _cgiInputFD(other._cgiInputFD),
      _timerId(other._timerId),
      _processTimerId(other._processTimerId) {
    DEBUG("CGIResponse copied for client: " << _client.get());
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
void CGIResponse::_setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl) {
    (void)route;
    _environmentVariables.clear();
    _environmentVariables["SERVER_SOFTWARE"] = "webserv/1.0";
    _environmentVariables["SERVER_NAME"] = std::string(config.serverName.getServerName());
    _environmentVariables["GATEWAY_INTERFACE"] = "CGI/1.1";

    _environmentVariables["SERVER_PROTOCOL"] = Response::protocol + std::string("/") + Response::tlsVersion;
    _environmentVariables["SERVER_PORT"] = std::to_string(config.port.getPort());
    _environmentVariables["REQUEST_METHOD"] = methodToStr(_client.get()->request.metadata.getMethod());
    _environmentVariables["PATH_INFO"] = std::string(parsedUrl.pathInfo);
    _environmentVariables["PATH_TRANSLATED"] = std::string(parsedUrl.scriptPath + parsedUrl.pathInfo);
    _environmentVariables["SCRIPT_FILENAME"] = parsedUrl.scriptPath;
    _environmentVariables["SCRIPT_NAME"] = Path(parsedUrl.scriptPath).getFilename();
    _environmentVariables["QUERY_STRING"] = std::string(parsedUrl.query);
    _environmentVariables["REMOTE_ADDR"] = _client.get()->getClientIP();
    _environmentVariables["REMOTE_PORT"] = _client.get()->getClientPort();
    _environmentVariables["SERVER_ADDR"] = _client.get()->getServer().getServerAddress();
    _environmentVariables["SERVER_PORT"] = std::to_string(config.port.getPort());
    _environmentVariables["REDIRECT_STATUS"] = "200";

    if (_client.get()->request.session && !_client.get()->request.session->sessionId.empty())
        _environmentVariables["HTTP_SESSION_FILE"] = _client.get()->request.session->absoluteFilePath;

    const std::string contentHeader = _client.get()->request.headers.getHeader(HeaderKey::ContentType, "");
    if (!contentHeader.empty())
        _environmentVariables["CONTENT_TYPE"] = contentHeader;

    const std::string contentLength = _client.get()->request.headers.getHeader(HeaderKey::ContentLength, "");
    if (!contentLength.empty())
        _environmentVariables["CONTENT_LENGTH"] = contentLength;

    for (const auto &[headerKey, headerValue] : _client.get()->request.headers.getHeaders()) {
        std::string envVarKey = "HTTP_" + headerKey;
        std::replace(envVarKey.begin(), envVarKey.end(), '-', '_');
        std::transform(envVarKey.begin(), envVarKey.end(), envVarKey.begin(), ::toupper);
        _environmentVariables[envVarKey] = headerValue;
    }
}

char * const *CGIResponse::_createEnvironmentArray() const {
    static std::vector<std::string> envStrings(_environmentVariables.size());
    static std::vector<char*> envPtrs;
    envPtrs.clear();
    for (auto &env : _environmentVariables) {
        envStrings.push_back(env.first + "=" + env.second);
        envPtrs.push_back(const_cast<char*>(envStrings.back().c_str()));
    }
    envPtrs.push_back(nullptr);
    return envPtrs.data();
}

void CGIResponse::start(const ServerConfig &config, const LocationRule &route) {
    DEBUG("Starting CGIResponse for client: " << _client.get());
    const ParsedUrl parsedUrl = parseUrl(route, _client.get()->request.metadata);

    DEBUG("Parsed URL: scriptPath=" << parsedUrl.scriptPath
          << ", pathInfo=" << parsedUrl.pathInfo
          << ", query=" << parsedUrl.query
          << ", isValid=" << parsedUrl.isValid);

    if (!parsedUrl.isValid) {
        DEBUG("Unknown URL for CGI processing: " << _client.request.metadata.getRawUrl());
        _innerStatusCode = HttpStatusCode::NotFound;
        return;
    }

    if (!(route.cgiExtension.isCGI(Path(parsedUrl.scriptPath)) || route.cgi.isSet())) {
        ERROR("The requested path is not a CGI script: " << parsedUrl.scriptPath);
        _innerStatusCode = HttpStatusCode::Forbidden;
        return;
    }

    _setupEnvironmentVariables(config, route, parsedUrl);

    int cin[2], cout[2];
    if (pipe(cin) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
        _innerStatusCode = HttpStatusCode::InternalServerError;
        return;
    }

    if (pipe(cout) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
        close(cin[0]);
        close(cin[1]);
        _innerStatusCode = HttpStatusCode::InternalServerError;
        return;
    }

    _processTimerId = fork();
    if (_processTimerId < 0) {
        ERROR("Failed to fork CGI process for client: " << &_client);
        close(cin[0]);
        close(cin[1]);
        close(cout[0]);
        close(cout[1]);
        _innerStatusCode = HttpStatusCode::InternalServerError;
        return;
    }

    if (_processTimerId == 0) {
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

        if (execve(scriptName.c_str(), argv, _createEnvironmentArray()) == -1) {
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
            _cgiInputFD.close();
            _cgiOutputFD.close();
            _innerStatusCode = HttpStatusCode::InternalServerError;
            return;
        }

        if (_cgiInputFD.connectToEpoll(_server.getEpollFd(), DEFAULT_EPOLLOUT_EVENTS) == -1 ||
            _cgiOutputFD.connectToEpoll(_server.getEpollFd(), DEFAULT_EPOLLIN_EVENTS) == -1) {
            ERROR("Failed to connect CGI pipes to epoll: " << strerror(errno));
            _cgiInputFD.close();
            _cgiOutputFD.close();
            _innerStatusCode = HttpStatusCode::InternalServerError;
            return;
        }

        _processTimerId = _server.getTimer().addEvent(std::chrono::milliseconds(static_cast<int>(route.cgiTimeout.timeout.getSeconds() * 1000.0)), [this]() {
            _handleTimeout();
        });

        _server.trackCallbackFD(_cgiInputFD, [this](WritableFD &fd, short revents) {
            DEBUG("CGIResponse read callback, fd: " << fd.get() << ", revents: " << revents);
            _handleCGIInputCallback(fd, revents);

        });
        _server.trackCallbackFD(_cgiOutputFD, [this](ReadableFD &fd, short revents) {
            DEBUG("CGIResponse write callback, fd: " << fd.get() << ", revents: " << revents);
            _handleCGIOutputCallback(fd, revents);
        });

        DEBUG("CGI process started with PID: " << _processTimerId << ", toCGIProcessFd: " << _toCGIProcessFd << ", fromCGIProcessFd: " << _fromCGIProcessFd);
    }
}

void CGIResponse::_handleCGIInputCallback(WritableFD &fd, short revents) {
    DEBUG("CGIResponse handleCGIInputCallback called, fd: " << fd.get() << ", revents: " << revents);
    
    if (revents & EPOLLOUT) {
        if (_socketFD.getReadBufferSize() == 0) return ;

        if (_client.get()->request.receivingBodyMode == ReceivingBodyMode::Chunked) {
            while (true) {
                ReadableFD::HTTPChunk chunk = _socketFD.extractHTTPChunkFromReadBuffer();
                if (chunk.size == ReadableFD::HTTPChunk::noChunk) return ;
                if (chunk.size == 0) {
                    _chunkedResponseDone = true;
                    _server.untrackCallbackFD(fd);
                    _cgiInputFD.close();
                    return ;
                }

                if (fd.writeAsString(chunk.data) < 0) {
                    ERROR("Failed to write chunk to CGI process");
                    _innerStatusCode = HttpStatusCode::InternalServerError;
                    _server.untrackCallbackFD(fd);
                    _cgiInputFD.close();
                    return ;
                }
            }
        } else {
            if (fd.writeAsString(_socketFD.extractFullBuffer()) < 0) {
                ERROR("Failed to write request body to CGI process");
                _innerStatusCode = HttpStatusCode::InternalServerError;
                _server.untrackCallbackFD(fd);
                _cgiInputFD.close();
                return ;
            }
        }
    }
}

void CGIResponse::_handleCGIOutputCallback(ReadableFD &fd, short revents) {
    DEBUG("CGIResponse handleCGIOutputCallback called, fd: " << fd.get() << ", revents: " << revents);

    if (fd.wouldReadExceedMaxBufferSize() && _socketFD.getWriterFDState() == FDState::Ready) {
        DEBUG("CGI output buffer would exceed max size, sending and praying");
    }
    
    if (revents & EPOLLIN) {
        std::string cgiOutput = fd.extractFullBuffer();
        if (cgiOutput.empty()) {
            DEBUG("No output from CGI process, closing output FD");
            _server.untrackCallbackFD(fd);
            _cgiOutputFD.close();
            return ;
        }

        _client.get()->response.updateFromCGIOutput(cgiOutput);
        if (_client.get()->response.didResponseCreationFail()) {
            _innerStatusCode = _client.get()->response.getFailedResponseStatusCode();
            _server.untrackCallbackFD(fd);
            _cgiOutputFD.close();
            return ;
        }
    }
}

void CGIResponse::_handleTimeout() {
    DEBUG("toCGIProcessFd: " << _toCGIProcessFd << ", fromCGIProcessFd: " << _fromCGIProcessFd << ", pid shit: " << _processTimerId);

    DEBUG("CGIResponse timeout handler called for client: " << &_client);
    if (didResponseCreationFail()) {
        exitProcess();
        _client.response.setStatusCode(HttpStatusCode::GatewayTimeout);
        _client.handleCGIResponse();
    }
}

void CGIResponse::exitProcess() {
    if (_isRunning) {
        _isRunning = false;

        if (_toCGIProcessFd != -1) __server.untrackClient(_toCGIProcessFd);
        if (_fromCGIProcessFd != -1) __server.untrackClient(_fromCGIProcessFd);

        _toCGIProcessFd = -1;
        _fromCGIProcessFd = -1;

        kill(_processTimerId, SIGKILL);
    }
}

void CGIResponse::handleDisconnectCallback(FD &fd) {
    DEBUG("toCGIProcessFd: " << _toCGIProcessFd << ", fromCGIProcessFd: " << _fromCGIProcessFd << ", pid shit: " << _processTimerId);
    if (fd.get() == _toCGIProcessFd) {
        DEBUG("Set _toCGIProcessFd to -1, fd: " << fd.get());
        _toCGIProcessFd = -1;
        return ;
    }

    if (fd.get() != _fromCGIProcessFd) return ;
    DEBUG("Set _fromCGIProcessFd to -1, fd: " << fd.get());
    _fromCGIProcessFd = -1;

    if (_isRunning) {
        int exitStatus = 0;
        waitpid(_processTimerId, &exitStatus, 0);
        DEBUG("CGIResponse exited normally, PID: " << _processTimerId << ", exit status: " << WEXITSTATUS(exitStatus));
        _isRunning = false;
        __server.getTimer().deleteEvent(_processTimerId);
        if (exitStatus == 0) _client.response.updateFromCGIOutput(fd.readBuffer);
        else _client.response.setStatusCode(HttpStatusCode::InternalServerError);
        DEBUG(fd.readBuffer);
        _client.handleCGIResponse();
    }
    DEBUG("CGIResponse disconnect callback, fd: " << fd.get());
}

void CGIResponse::handleReadCallback(FD &fd, int funcReturnValue) {
    DEBUG("CGIResponse read callback, fd: " << fd.get() << ", funcReturnValue: " << funcReturnValue);
    (void)funcReturnValue;
    (void)fd;
}

void CGIResponse::handleWriteCallback(FD &fd) {
    if (fd.get() == _toCGIProcessFd) {
        DEBUG("Writing request body to CGI process, fd: " << fd.get());
        fd.writeToBuffer(_client.request.getBody());
        fd.write();
        __server.untrackClient(fd.get());
        DEBUG("Set _toCGIProcessFd to -1");
        _toCGIProcessFd = -1;
    }
}