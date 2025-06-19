#include "config/rules/rules.hpp"
#include "print.hpp"
#include "CGI.hpp"

#include <filesystem>

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

CGIClient::CGIClient(Client &client) : _client(client), _isRunning(true), _pid(-1), _environmentVariables() {
    DEBUG("CGIClient created for client: " << &_client);
}

/// @brief Sets up the environment variables for the CGI process.
/// @details https://www6.uniovi.es/~antonio/ncsa_httpd/cgi/env.html
/// @param config The server configuration
/// @param route The location rule
void CGIClient::_setupEnvironmentVariables(const ServerConfig &config, const LocationRule &route, const ParsedUrl &parsedUrl) {
    _environmentVariables.clear();
    _environmentVariables["SERVER_SOFTWARE"] = "webserv/1.0";
    _environmentVariables["SERVER_NAME"] = config.serverName.get();
    _environmentVariables["GATEWAY_INTERFACE"] = "CGI/1.1";

    _environmentVariables["SERVER_PROTOCOL"] = Response::protocol + std::string("/") + Response::tlsVersion;
    _environmentVariables["SERVER_PORT"] = std::to_string(config.port.get());
    _environmentVariables["REQUEST_METHOD"] = methodToStr(_client.request.metadata.getMethod());
    _environmentVariables["PATH_INFO"] = parsedUrl.pathInfo;
    _environmentVariables["PATH_TRANSLATED"] = parsedUrl.scriptPath + parsedUrl.pathInfo;
    _environmentVariables["SCRIPT_NAME"] = parsedUrl.scriptPath;
    _environmentVariables["QUERY_STRING"] = parsedUrl.query;
    _environmentVariables["REMOTE_ADDR"] = _client.getClientIP();
    _environmentVariables["REMOTE_PORT"] = _client.getClientPort();

    const std::string &contentHeader = _client.request.headers.getHeader(HeaderKey::ContentType, "");
    if (!contentHeader.empty())
        _environmentVariables["CONTENT_TYPE"] = contentHeader;

    const std::string &contentLength = _client.request.headers.getHeader(HeaderKey::ContentLength, "");
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
    static std::vector<std::string> envVars;
    envVars.clear();

    for (const auto &[key, value] : _environmentVariables) {
        envVars.push_back(key + "=" + value);
    }
    envVars.push_back(nullptr); // Null-terminate the array
    return envVars.empty() ? nullptr : reinterpret_cast<char * const *>(envVars.data());
}

void CGIClient::start(const ServerConfig &config, const LocationRule &route) {
    DEBUG("Starting CGIClient for client: " << &_client);
    ParsedUrl parsedUrl = parseUrl(_client.request.metadata.getPath(), route);
    if (!parsedUrl.isValid) {
        DEBUG("Invalid URL for CGI processing: " << _client.request.metadata.getPath());
        _client.response.setStatusCode(HttpStatusCode::BadRequest);
        _isRunning = false;
        return;
    }

    _setupEnvironmentVariables(config, route, parsedUrl);

    _pid = fork();
    if (_pid < 0) {
        ERROR("Failed to fork CGI process for client: " << &_client);
        _client.response.setStatusCode(HttpStatusCode::InternalServerError);
        _isRunning = false;
        return;
    }

    int cin[2], cout[2];
    if (pipe(cin) == -1 || pipe(cout) == -1) {
        ERROR("Failed to create pipes for CGI process: " << strerror(errno));
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

        if (execve(parsedUrl.scriptPath.c_str(), nullptr, _createEnvironmentArray()) == -1) {
            ERROR("Failed to execute CGI script: " << parsedUrl.scriptPath << ", errno: " << errno << " (" << strerror(errno) << ")");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        std::shared_ptr<CGIClient> cgiClient = std::make_shared<CGIClient>(*this);
        FD toCGIProccess = FD::fromPipeWriteEnd(cin, cgiClient);
        FD fromCGIProccess = FD::fromPipeReadEnd(cout, cgiClient);
    }
}