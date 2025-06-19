#include "config/rules/rules.hpp"
#include "timer.hpp"
#include "print.hpp"
#include "CGI.hpp"

#include <filesystem>
#include <signal.h>
#include <chrono>

/// @brief Parses the given URL and extracts the script path, path info, and query string.
/// @param url The URL to parse, which may include a query string.
/// @param route The location rule to use for parsing the URL.
/// @return A ParsedUrl struct containing the extracted information.
static ParsedUrl parseUrl(const std::string &url, const LocationRule &route) {
    std::string baseUrl = url.substr(0, url.find('?'));
    Path path = Path::createFromUrl(baseUrl, route);

    Path tmp = path;
    while (!tmp.str().empty()) {
        if (std::filesystem::exists(tmp.str()) && std::filesystem::is_regular_file(tmp.str())) {
            DEBUG("Found CGI script: " << tmp.str());
            return (ParsedUrl{
                .scriptPath = tmp.str(),
                .pathInfo = path.str().substr(tmp.str().length()),
                .query = (url.find('?') != std::string::npos ? url.substr(url.find('?') + 1) : ""),
                .isValid = true
            });
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

CGIClient::CGIClient(Client &client) : _client(client), _processTimerId(-1), _isRunning(true), _pid(-1), _toCGIProcessFd(-1), _fromCGIProcessFd(-1), _environmentVariables() {
    DEBUG("CGIClient created for client: " << &_client);
}

CGIClient::CGIClient(const CGIClient &other) : _client(other._client), _processTimerId(other._processTimerId), _isRunning(other._isRunning), _pid(other._pid), _toCGIProcessFd(other._toCGIProcessFd), _fromCGIProcessFd(other._fromCGIProcessFd), _environmentVariables(other._environmentVariables) {
    DEBUG("CGIClient copied for client: " << &_client);
}

/// @brief Sets up the environment variables for the CGI process.
/// @details https://www6.uniovi.es/~antonio/ncsa_httpd/cgi/env.html
/// @param config The server configuration
/// @param route The location rule
void CGIClient::_setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl) {
    (void)route;
    _environmentVariables.clear();
    _environmentVariables["SERVER_SOFTWARE"] = "webserv/1.0";
    _environmentVariables["SERVER_NAME"] = std::string(config.serverName.get());
    _environmentVariables["GATEWAY_INTERFACE"] = "CGI/1.1";

    _environmentVariables["SERVER_PROTOCOL"] = Response::protocol + std::string("/") + Response::tlsVersion;
    _environmentVariables["SERVER_PORT"] = std::to_string(config.port.get());
    _environmentVariables["REQUEST_METHOD"] = methodToStr(_client.request.metadata.getMethod());
    _environmentVariables["PATH_INFO"] = std::string(parsedUrl.pathInfo);
    _environmentVariables["PATH_TRANSLATED"] = std::string(parsedUrl.scriptPath + parsedUrl.pathInfo);
    _environmentVariables["SCRIPT_NAME"] = std::string(parsedUrl.scriptPath);
    _environmentVariables["QUERY_STRING"] = std::string(parsedUrl.query);
    _environmentVariables["REMOTE_ADDR"] = _client.getClientIP();
    _environmentVariables["REMOTE_PORT"] = _client.getClientPort();

    const std::string contentHeader = _client.request.headers.getHeader(HeaderKey::ContentType, "");
    if (!contentHeader.empty())
        _environmentVariables["CONTENT_TYPE"] = contentHeader;

    const std::string contentLength = _client.request.headers.getHeader(HeaderKey::ContentLength, "");
    if (!contentLength.empty())
        _environmentVariables["CONTENT_LENGTH"] = contentLength;

    for (const auto &[headerKey, headerValue] : _client.request.headers.getHeaders()) {
        std::string envVarKey = "HTTP_" + headerKey;
        std::replace(envVarKey.begin(), envVarKey.end(), '-', '_');
        std::transform(envVarKey.begin(), envVarKey.end(), envVarKey.begin(), ::toupper);
        _environmentVariables[envVarKey] = headerValue;
    }
}

char * const *CGIClient::_createEnvironmentArray() const {
    static std::vector<char*> envPtrs;
    envPtrs.clear();
    for (auto &env : _environmentVariables) {
        envPtrs.push_back(const_cast<char*>(env.first.c_str()));
    }
    envPtrs.push_back(nullptr); // Null-terminate the array
    return envPtrs.data();
}

void CGIClient::start(const ServerConfig &config, const LocationRule &route) {
    DEBUG("Starting CGIClient for client: " << &_client);
    const ParsedUrl parsedUrl = parseUrl(_client.request.metadata.getPath(), route);

    DEBUG("Parsed URL: scriptPath=" << parsedUrl.scriptPath
          << ", pathInfo=" << parsedUrl.pathInfo
          << ", query=" << parsedUrl.query
          << ", isValid=" << parsedUrl.isValid);

    if (!parsedUrl.isValid) {
        DEBUG("Invalid URL for CGI processing: " << _client.request.metadata.getPath());
        _client.response.setStatusCode(HttpStatusCode::BadRequest);
        _isRunning = false;
        return;
    }

    _setupEnvironmentVariables(config, route, parsedUrl);

    int cin[2], cout[2];
    if (pipe(cin) == -1 || pipe(cout) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
        _client.response.setStatusCode(HttpStatusCode::InternalServerError);
        _isRunning = false;
        return;
    }

    _pid = fork();
    if (_pid < 0) {
        ERROR("Failed to fork CGI process for client: " << &_client);
        close(cin[0]);
        close(cin[1]);
        close(cout[0]);
        close(cout[1]);
        _client.response.setStatusCode(HttpStatusCode::InternalServerError);
        _isRunning = false;
        return;
    }

    if (_pid == 0) {
        // Child process
        FD childStdin = FD::fromPipeReadEnd(cin);
        FD childStdout = FD::fromPipeWriteEnd(cout);
        if (dup2(childStdin.get(), STDIN_FILENO) == -1 || dup2(childStdout.get(), STDOUT_FILENO) == -1) {
            childStdin.close();
            childStdout.close();
            ERROR("Failed to redirect stdin/stdout for CGI process: " << strerror(errno));
            exit(EXIT_FAILURE);
        }

        childStdin.close();
        childStdout.close();

        char * const argv[] = {const_cast<char *>(parsedUrl.scriptPath.c_str()), nullptr};
        if (execve(parsedUrl.scriptPath.c_str(), argv, _createEnvironmentArray()) == -1) {
            ERROR("Failed to execute CGI script: " << parsedUrl.scriptPath << ", errno: " << errno << " (" << strerror(errno) << ")");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        std::shared_ptr<BaseHandlerObject> cgiClient = shared_from_this();
        FD toCGIProccess = FD::fromPipeWriteEnd(cin, cgiClient);
        FD fromCGIProccess = FD::fromPipeReadEnd(cout, cgiClient);

        this->_toCGIProcessFd = toCGIProccess.get();
        this->_fromCGIProcessFd = fromCGIProccess.get();

        if (toCGIProccess.setNonBlocking() == -1 || fromCGIProccess.setNonBlocking() == -1) {
            ERROR("Failed to set non-blocking mode for CGI pipes: " << strerror(errno));
            toCGIProccess.close();
            fromCGIProccess.close();
            _client.response.setStatusCode(HttpStatusCode::InternalServerError);
            _isRunning = false;
            return;
        }

        if (toCGIProccess.connectToEpoll(_client.getServer().getEpollFd(), DEFAULT_EPOLLOUT_EVENTS) == -1 ||
            fromCGIProccess.connectToEpoll(_client.getServer().getEpollFd(), DEFAULT_EPOLLIN_EVENTS) == -1) {
            ERROR("Failed to connect CGI pipes to epoll: " << strerror(errno));
            toCGIProccess.close();
            fromCGIProccess.close();
            _client.response.setStatusCode(HttpStatusCode::InternalServerError);
            _isRunning = false;
            return;
        }

        _processTimerId = _client.getServer().getTimer().addEvent(std::chrono::milliseconds(static_cast<int>(route.cgiTimeout.get() * 1000.0)), [this]() {
            _handleTimeout();
        });

        _client.getServer().trackDescriptor(toCGIProccess);
        _client.getServer().trackDescriptor(fromCGIProccess);
        DEBUG("CGI process started with PID: " << _pid << ", toCGIProcessFd: " << _toCGIProcessFd << ", fromCGIProcessFd: " << _fromCGIProcessFd);
    }

        DEBUG("toCGIProcessFd: " << _toCGIProcessFd << ", fromCGIProcessFd: " << _fromCGIProcessFd << ", pid shit: " << _pid);
}

void CGIClient::_handleTimeout() {
    DEBUG("toCGIProcessFd: " << _toCGIProcessFd << ", fromCGIProcessFd: " << _fromCGIProcessFd << ", pid shit: " << _pid);

    DEBUG("CGIClient timeout handler called for client: " << &_client);
    if (_isRunning) {
        _isRunning = false;

        if (_toCGIProcessFd != -1) _client.getServer().untrackDescriptor(_toCGIProcessFd);
        if (_fromCGIProcessFd != -1) _client.getServer().untrackDescriptor(_fromCGIProcessFd);
        DEBUG("Set _toCGIProcessFd and _fromCGIProcessFd to -1");
        _toCGIProcessFd = -1;
        _fromCGIProcessFd = -1;

        kill(_pid, SIGKILL);
        _client.response.setStatusCode(HttpStatusCode::GatewayTimeout);
        _client.handleCGIResponse();
    }
}

void CGIClient::handleDisconnectCallback(FD &fd) {
    DEBUG("toCGIProcessFd: " << _toCGIProcessFd << ", fromCGIProcessFd: " << _fromCGIProcessFd << ", pid shit: " << _pid);
    if (fd.get() == _toCGIProcessFd) {
        DEBUG("Set _toCGIProcessFd to -1, fd: " << fd.get());
        _toCGIProcessFd = -1;
        return ;
    }

    if (fd.get() != _fromCGIProcessFd) return ;
    DEBUG("Set _fromCGIProcessFd to -1, fd: " << fd.get());
    _fromCGIProcessFd = -1;

    if (_isRunning) {
        DEBUG("CGIClient exited normally, PID: " << _pid);
        _client.response.setStatusCode(HttpStatusCode::OK);
        _client.response.setBody(fd.readBuffer);
        _client.handleCGIResponse();
        _isRunning = false;
    }
    DEBUG("CGIClient disconnect callback, fd: " << fd.get());
}

void CGIClient::handleReadCallback(FD &fd, int funcReturnValue) {
    DEBUG("CGIClient read callback, fd: " << fd.get() << ", funcReturnValue: " << funcReturnValue);
    (void)funcReturnValue;
    (void)fd;
}

void CGIClient::handleWriteCallback(FD &fd) {
    if (fd.get() == _toCGIProcessFd) {
        DEBUG("Writing request body to CGI process, fd: " << fd.get());
        fd.writeToBuffer(_client.request.getBody());
        fd.write();
        _client.getServer().untrackDescriptor(fd.get());
        DEBUG("Set _toCGIProcessFd to -1");
        _toCGIProcessFd = -1;
    }
}