#include "config/rules/rules.hpp"
#include "Utils.hpp"
#include "print.hpp"
#include "CGI.hpp"

#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>   // for getcwd
#include <limits.h>   // for PATH_MAX

CGI::CGI()
    : _env(),
      _scriptPath(),
      _interpreter(),
      _inputPipe{-1, -1},
      _outputPipe{-1, -1},
      _errorPipe{-1, -1},
      _pid(-1),
      _status(Status::RUNNING),
      _pipesClosed(false),
      _startTime(0),
      _output(),
      _error(),
      _executionFuture(),
      _outputPipeEOF(false),
      _errorPipeEOF(false)
{
    DEBUG("[CGI ctor] TIMEOUT_SECONDS=" << TIMEOUT_SECONDS);
}

CGI::~CGI() {
    closePipes();
    // Kill the CGI process if it's still running
    if (_pid > 0) {
        DEBUG("[CGI dtor] Killing pid " << _pid);
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
    }
}

void CGI::closePipes() {
    if (_pipesClosed) {
        DEBUG("[CGI closePipes] Already closed for pid=" << _pid);
        return;
    }
    _pipesClosed = true;
    DEBUG("[CGI closePipes] Closing pipes for pid=" << _pid << ", outputPipe[0]=" << _outputPipe[0] << ", errorPipe[0]=" << _errorPipe[0]);
    if (_inputPipe[0] != -1) { DEBUG("[CGI closePipes] Closing _inputPipe[0] fd=" << _inputPipe[0]); close(_inputPipe[0]); _inputPipe[0] = -1; }
    if (_inputPipe[1] != -1) { DEBUG("[CGI closePipes] Closing _inputPipe[1] fd=" << _inputPipe[1]); close(_inputPipe[1]); _inputPipe[1] = -1; }
    if (_outputPipe[0] != -1) { DEBUG("[CGI closePipes] Closing _outputPipe[0] fd=" << _outputPipe[0]); close(_outputPipe[0]); _outputPipe[0] = -1; }
    if (_outputPipe[1] != -1) { DEBUG("[CGI closePipes] Closing _outputPipe[1] fd=" << _outputPipe[1]); close(_outputPipe[1]); _outputPipe[1] = -1; }
    if (_errorPipe[0] != -1) { DEBUG("[CGI closePipes] Closing _errorPipe[0] fd=" << _errorPipe[0]); close(_errorPipe[0]); _errorPipe[0] = -1; }
    if (_errorPipe[1] != -1) { DEBUG("[CGI closePipes] Closing _errorPipe[1] fd=" << _errorPipe[1]); close(_errorPipe[1]); _errorPipe[1] = -1; }
    _outputPipeEOF = true;
    _errorPipeEOF = true;
}

bool CGI::startExecution(const Request& request, const LocationRule& route, const ServerConfig& server) {
    // Determine script path
    Path requestPath = Path::createFromUrl(request.metadata.getPath(), route);
    DEBUG("CGI: Request path: " << requestPath.str());

    if (!requestPath.isValid()) {
        DEBUG("CGI: Invalid request path: " << requestPath.str());
        return false;
    }

    // Check if file exists
    struct stat statBuf;
    if (stat(requestPath.str().c_str(), &statBuf) != 0) {
        DEBUG("CGI: Script not found: " << requestPath.str());
        return false;
    }

    if (!(statBuf.st_mode & S_IXUSR)) {
        DEBUG("CGI: Script not executable: " << requestPath.str());
        return false;
    }

    std::string extention = Utils::getFileExtension(requestPath.str());
    DEBUG("CGI: File extension: " << extention);

    struct stat interpreterStat;
    if (stat(_interpreter.c_str(), &interpreterStat) != 0) {
        DEBUG("CGI: Interpreter not found: " << _interpreter);
        return false;
    }

    setupEnvironment(request, route, server, _interpreter);
    DEBUG("CGI: Environment variables set up");

    // Create pipes
    if (pipe(_inputPipe) == -1 || pipe(_outputPipe) == -1 || pipe(_errorPipe) == -1) {
        ERROR("Failed to create pipes: " << strerror(errno));
        return false;
    }
    
    // Set pipes to non-blocking
    fcntl(_inputPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(_outputPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(_errorPipe[0], F_SETFL, O_NONBLOCK);
    
    // Start the CGI process
    _pid = fork();
    if (_pid == -1) {
        ERROR("Failed to fork: " << strerror(errno));
        return false;
    }
    
    if (_pid == 0) {
        // Child process
        DEBUG("CGI Child: Starting child process");
        DEBUG("CGI Child: Script path: " << _scriptPath);
        DEBUG("CGI Child: Interpreter: " << _interpreter);
        
        // Close unused pipe ends
        close(_inputPipe[1]);   // Close write end of input pipe
        close(_outputPipe[0]);  // Close read end of output pipe
        close(_errorPipe[0]);   // Close read end of error pipe
        
        // Redirect stdin, stdout, stderr
        if (dup2(_inputPipe[0], STDIN_FILENO) == -1 ||
            dup2(_outputPipe[1], STDOUT_FILENO) == -1 ||
            dup2(_errorPipe[1], STDERR_FILENO) == -1) {
            // Can't use std::cerr here since stderr is redirected
            ERROR("Failed to redirect file descriptors: " << strerror(errno));
            exit(1);
        }
        
        // Close original pipe file descriptors
        close(_inputPipe[0]);
        close(_outputPipe[1]);
        close(_errorPipe[1]);
        
        // Change to script directory
        std::string workDir = getWorkingDirectory(_scriptPath);
        if (chdir(workDir.c_str()) == -1) {
            write(STDERR_FILENO, "Failed to change directory\n", 27);
            exit(1);
        }
        
        // Set up environment
        std::vector<std::string> envStrings;
        std::vector<char*> envp;

        for (const auto& pair : _env) {
            envStrings.push_back(pair.first + "=" + pair.second);
        }
        for (size_t i = 0; i < envStrings.size(); ++i) {
            envp.push_back(const_cast<char*>(envStrings[i].c_str()));
        }
        envp.push_back(nullptr);

        // Use envp for execve or similar
        // No manual delete needed for envStrings, since std::string manages memory.

        
        // Prepare arguments
        std::vector<char*> argv;
        
        // Add interpreter
        char* interpreterStr = new char[_interpreter.length() + 1];
        strcpy(interpreterStr, _interpreter.c_str());
        argv.push_back(interpreterStr);
        
        // Add script path (use relative path from working directory)
        std::string scriptName = _scriptPath.substr(_scriptPath.find_last_of('/') + 1);
        char* scriptStr = new char[scriptName.length() + 1];
        strcpy(scriptStr, scriptName.c_str());
        argv.push_back(scriptStr);
        
        argv.push_back(NULL);
        
        // Execute script
        execve(_interpreter.c_str(), &argv[0], &envp[0]);

        // If we reach here, execve failed
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "execve failed: errno=%d\n", errno);
        write(STDERR_FILENO, errorMsg, strlen(errorMsg));
        exit(1);
    } else {
        // Parent process
        // Close unused pipe ends
        close(_inputPipe[0]);   // Close read end of input pipe
        close(_outputPipe[1]);  // Close write end of output pipe
        close(_errorPipe[1]);   // Close write end of error pipe
        
        // Write request body to child's stdin
        if (!request.getBody().empty()) {
            const std::string& input = request.getBody();
            size_t totalWritten = 0;
            while (totalWritten < input.length()) {
                ssize_t written = write(_inputPipe[1], input.c_str() + totalWritten, 
                                      input.length() - totalWritten);
                if (written == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // Would block, try again later
                        usleep(1000); // Sleep for 1ms
                        continue;
                    } else {
                        ERROR("Failed to write to CGI stdin: " << strerror(errno));
                        break;
                    }
                } else {
                    totalWritten += written;
                }
            }
        }
        close(_inputPipe[1]); // Close stdin to signal EOF
        DEBUG("[CGI parent] Closed _inputPipe[1] (fd=" << _inputPipe[1] << ") for pid " << _pid);
        _inputPipe[1] = -1;
        // Record start time
        _startTime = time(NULL);
        _status = Status::RUNNING;
        return true;
    }
}

CGI::Status CGI::updateExecution() {
    if (_status != Status::RUNNING) {
        // Only close pipes if process is not running AND both pipes are closed
        if (_outputPipe[0] == -1 && _errorPipe[0] == -1) {
            closePipes();
        }
        return _status;
    }
    // Check for timeout
    if (time(NULL) - _startTime > TIMEOUT_SECONDS) {
        ERROR("CGI script timeout");
        kill(_pid, SIGKILL);
        _status = Status::TIMEOUT;
        // Only close pipes if both are closed
        if (_outputPipe[0] == -1 && _errorPipe[0] == -1) {
            closePipes();
        }
        return _status;
    }
    // Read from pipes
    readFromPipes();
    // Check if process has finished
    bool finished = checkProcessStatus();
    // Only close pipes if process is finished AND both pipes are closed
    if (finished && _outputPipe[0] == -1 && _errorPipe[0] == -1) {
        closePipes();
    }
    return _status;
}

bool CGI::readFromPipes() {
    if (_pipesClosed) {
        DEBUG("[CGI readFromPipes] Called after pipes closed for pid=" << _pid);
        return false;
    }
    char buffer[4096];
    ssize_t bytesRead;
    // Read from stdout
    if (_outputPipe[0] != -1 && !_outputPipeEOF) {
        while ((bytesRead = read(_outputPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            DEBUG("[CGI readFromPipes] Read " << bytesRead << " bytes from _outputPipe[0] fd=" << _outputPipe[0] << ", pid=" << _pid);
            _output.append(buffer, bytesRead);
            if (_output.length() > MAX_OUTPUT_SIZE) {
                ERROR("CGI output too large");
                kill(_pid, SIGKILL);
                _status = Status::ERROR;
                return false;
            }
        }
        if (bytesRead == 0) {
            DEBUG("[CGI readFromPipes] EOF on _outputPipe[0] fd=" << _outputPipe[0] << ", pid=" << _pid);
            _outputPipeEOF = true;
            // Do NOT close(_outputPipe[0]) here!
        } else if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available, not an error in non-blocking mode
            } else if (errno == EBADF) {
                ERROR("[CGI readFromPipes] EBADF on _outputPipe[0] for CGI instance " << this << ", pid=" << _pid);
                _outputPipeEOF = true;
                if (_outputPipe[0] != -1) { close(_outputPipe[0]); _outputPipe[0] = -1; }
                return false;
            } else {
                ERROR("[CGI readFromPipes] Unexpected read() error " << errno << " on _outputPipe[0] for CGI instance " << this << ", pid=" << _pid);
                _outputPipeEOF = true;
                // Do NOT close(_outputPipe[0]) here!
            }
        }
    }
    // Read from stderr
    if (_errorPipe[0] != -1 && !_errorPipeEOF) {
        while ((bytesRead = read(_errorPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            DEBUG("[CGI readFromPipes] Read " << bytesRead << " bytes from _errorPipe[0] fd=" << _errorPipe[0] << ", pid=" << _pid);
            _error.append(buffer, bytesRead);
        }
        if (bytesRead == 0) {
            DEBUG("[CGI readFromPipes] EOF on _errorPipe[0] fd=" << _errorPipe[0] << ", pid=" << _pid);
            _errorPipeEOF = true;
            // Do NOT close(_errorPipe[0]) here!
        } else if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available, not an error in non-blocking mode
            } else if (errno == EBADF) {
                ERROR("[CGI readFromPipes] EBADF on _errorPipe[0] for CGI instance " << this << ", pid=" << _pid);
                _errorPipeEOF = true;
                if (_errorPipe[0] != -1) { close(_errorPipe[0]); _errorPipe[0] = -1; }
                return false;
            } else {
                ERROR("[CGI readFromPipes] Unexpected read() error " << errno << " on _errorPipe[0] for CGI instance " << this << ", pid=" << _pid);
                _errorPipeEOF = true;
                // Do NOT close(_errorPipe[0]) here!
            }
        }
    }
    return true;
}

bool CGI::checkProcessStatus() {
    int status;
    pid_t result = waitpid(_pid, &status, WNOHANG);
    if (result == _pid) {
        // Process has finished
        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            DEBUG("[CGI checkProcessStatus] Process exited with code: " << exitCode);
            if (exitCode != 0) {
                ERROR("CGI script exited with code: " << exitCode);
                if (!_error.empty()) {
                    ERROR("CGI stderr: " << _error);
                }
                _status = Status::ERROR;
            }
        } else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            ERROR("CGI script terminated by signal: " << signal);
            _status = Status::ERROR;
        }
        DEBUG("[CGI checkProcessStatus] Process " << _pid << " finished normally");
        _status = Status::FINISHED;
        // Only now close pipes if both EOFs have been seen
        if (_outputPipeEOF && _outputPipe[0] != -1) { close(_outputPipe[0]); _outputPipe[0] = -1; }
        if (_errorPipeEOF && _errorPipe[0] != -1) { close(_errorPipe[0]); _errorPipe[0] = -1; }
        return true;
    } else if (result == -1 && errno != ECHILD) {
        ERROR("waitpid error: " << strerror(errno));
        _status = Status::ERROR;
        return true;
    }
    return false;
}

bool CGI::getResponse(Response& response) {
    if (_status != Status::FINISHED) {
        DEBUG("[CGI getResponse] Not finished: status=" << static_cast<int>(_status) << ", pid=" << _pid);
        return false;
    }
    DEBUG("[CGI getResponse] Output buffer size for pid=" << _pid << ": " << _output.size());
    if (!_output.empty()) {
        DEBUG("[CGI getResponse] Output buffer (first 200 chars): " << _output.substr(0, 200));
    } else {
        DEBUG("[CGI getResponse] Output buffer is EMPTY for pid=" << _pid);
    }
    return parseOutput(response);
}

std::vector<int> CGI::getFileDescriptors() const {
    std::vector<int> fds;
    if (_outputPipe[0] != -1) fds.push_back(_outputPipe[0]);
    if (_errorPipe[0] != -1) fds.push_back(_errorPipe[0]);
    return fds;
}

void CGI::setupEnvironment(const Request& request, const LocationRule& route, const ServerConfig& server, const std::string &interpreterPath) {
    _env.clear();
    (void)route; // Unused parameter, but kept for consistency with the method signature
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        perror("getcwd() error");
        // handle error or fallback
    }
    std::string currentDir(cwd);

    std::string documentRoot = currentDir + "/" ;
    Path scriptPathObj(documentRoot);
    scriptPathObj.append(Path::createFromUrl(request.metadata.getPath(), route).str());
    DEBUG("CGI: Script path: " << scriptPathObj.str());
    _scriptPath = scriptPathObj.str();

    Path interpreterPathObj(documentRoot);
    interpreterPathObj.append(interpreterPath);

    _env["SCRIPT_FILENAME"] = scriptPathObj.str();
    _env["REQUEST_METHOD"] = methodToStr(request.metadata.getMethod());
    _env["SCRIPT_NAME"] = request.metadata.getPath();
    _env["PATH_INFO"] = request.metadata.getPath();
    _env["PATH_TRANSLATED"] = interpreterPathObj.str().c_str();
    _env["REQUEST_URI"] = request.metadata.getPath();
    _env["SERVER_NAME"] = server.serverName.get().empty() ? "Whatever" : server.serverName.get();
    _env["SERVER_PORT"] = Utils::intToString(server.port.get());
    _env["SERVER_PROTOCOL"] = Response::protocol;
    _env["SERVER_SOFTWARE"] = "webserv/1.0";
    _env["GATEWAY_INTERFACE"] = Response::tlsVersion;
    
    // IMPORTANT: PHP CGI security requirement
    _env["REDIRECT_STATUS"] = "200";
    
    // Query string
    size_t queryPos = request.metadata.getPath().find('?');
    if (queryPos != std::string::npos) {
        _env["QUERY_STRING"] = request.metadata.getPath().substr(queryPos + 1);
        _env["SCRIPT_NAME"] = request.metadata.getPath().substr(0, queryPos);
    } else {
        _env["QUERY_STRING"] = "";
    }
    
    // Content length and type for POST requests
    if (request.metadata.getMethod() == Method::POST) {
        _env["CONTENT_LENGTH"] = Utils::intToString(request.getBody().length());
        std::string contentType = request.headers.getHeader(HeaderKey::ContentType, "");
        if (!contentType.empty()) {
            _env["CONTENT_TYPE"] = contentType;
        }
    } else {
        _env["CONTENT_LENGTH"] = "0";
    }

    _env["HTTP_SESSION_FILE"] = request.session ? ("../../../sessions8080/" + request.session->getSessionId() + ".json") : "";

    // HTTP headers as environment variables
    std::string host = request.headers.getHeader(HeaderKey::Host, "");
    if (!host.empty()) {
        _env["HTTP_HOST"] = host;
    }

    std::string userAgent = request.headers.getHeader(HeaderKey::UserAgent, "");
    if (!userAgent.empty()) {
        _env["HTTP_USER_AGENT"] = userAgent;
    }

    std::string accept = request.headers.getHeader(HeaderKey::Accept, "");
    if (!accept.empty()) {
        _env["HTTP_ACCEPT"] = accept;
    }

    std::string acceptLanguage = request.headers.getHeader(HeaderKey::AcceptLanguage, "");
    if (!acceptLanguage.empty()) {
        _env["HTTP_ACCEPT_LANGUAGE"] = acceptLanguage;
    }

    std::string acceptEncoding = request.headers.getHeader(HeaderKey::AcceptEncoding, "");
    if (!acceptEncoding.empty()) {
        _env["HTTP_ACCEPT_ENCODING"] = acceptEncoding;
    }
    
    std::string cookie = request.headers.getHeader(HeaderKey::Cookie, "");
    if (!cookie.empty()) {
        _env["HTTP_COOKIE"] = cookie;
    }
    
    std::string referer = request.headers.getHeader(HeaderKey::Referer, "");
    if (!referer.empty()) {
        _env["HTTP_REFERER"] = referer;
    }
    
    // Remote address (simplified - you might want to get actual client IP)
    _env["REMOTE_ADDR"] = "127.0.0.1";
    _env["REMOTE_HOST"] = "localhost";
    
    // Working directory
    _env["PWD"] = getWorkingDirectory(_scriptPath);
}

bool CGI::parseOutput(Response& response) {
    if (_output.empty()) {
        ERROR("Empty CGI output");
        if (!_error.empty()) {
            response.setStatusCode(HttpStatusCode::InternalServerError);
            response.setBody("<html><body><h1>500 Internal Server Error</h1><pre>" + _error + "</pre></body></html>");
            response.headers.replace(HeaderKey::ContentType, "text/html");
            return true;
        }
        return false;
    }
    
    DEBUG("CGI: Parsing output (first 200 chars): " << _output.substr(0, 200));
    
    // Find the header/body separator and its length
    size_t separatorPos = _output.find("\r\n\r\n");
    size_t separatorLength = 4;
    
    if (separatorPos == std::string::npos) {
        separatorPos = _output.find("\n\n");
        separatorLength = 2;
        
        if (separatorPos == std::string::npos) {
            // No headers found, treat entire output as body
            response.setStatusCode(HttpStatusCode::OK);
            response.setBody(_output);
            return true;
        }
    }
    
    // Extract headers and body
    std::string headers = _output.substr(0, separatorPos);
    std::string body = _output.substr(separatorPos + separatorLength);
    
    std::istringstream headerStream(headers);
    std::string line;
    
    bool statusSet = false;
    
    while (std::getline(headerStream, line)) {
        line = Utils::trim(line);
        if (line.empty()) {
            continue;
        }
        
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            ERROR("Invalid CGI header: " << line);
            continue;
        }
        
        std::string name = Utils::trim(line.substr(0, colonPos));
        std::string value = Utils::trim(line.substr(colonPos + 1));
        
        DEBUG("CGI: Header - " << name << ": " << value);
        
        // Handle special headers
        if (Utils::toLower(name) == "status") {
            int statusCode = Utils::stringToInt(value.substr(0, 3));
            response.setStatusCode(static_cast<HttpStatusCode>(statusCode));
            statusSet = true;
        } else if (Utils::toLower(name) == "location") {
            response.headers.replace(HeaderKey::Location, value);
            if (!statusSet) {
                response.setStatusCode(HttpStatusCode::MovedPermanently); // Default redirect status
            }
        } else {
            response.headers.add(name, value);
        }
    }
    
    // Set default status if not set
    if (!statusSet) {
        response.setStatusCode(HttpStatusCode::OK);
    }
    
    // Set response body
    response.setBody(body);

    return true;
}

std::string CGI::getWorkingDirectory(const std::string& scriptPath) {
    size_t lastSlash = scriptPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        return scriptPath.substr(0, lastSlash);
    }
    return ".";
}