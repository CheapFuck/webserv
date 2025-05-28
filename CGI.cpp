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

CGI::CGI() {
}

CGI::~CGI() {
}

bool CGI::execute(const Request& request, const LocationRule& route, const ServerConfig& server, Response& response) {
    // Determine script path
    Path requestPath = Path::createFromUrl(request.metadata.getPath(), route);
    DEBUG("CGI: Request path: " << requestPath.str());

    if (!requestPath.isValid()) {
        DEBUG("CGI: Invalid request path: " << requestPath.str());
        response.setStatusCode(HttpStatusCode::BadRequest);
        return (false);
    }

    // Check if file exists
    struct stat statBuf;
    if (stat(requestPath.str().c_str(), &statBuf) != 0) {
        DEBUG("CGI: Script not found: " << requestPath.str());
        response.setStatusCode(HttpStatusCode::NotFound);
        return false;
    }

    if (!(statBuf.st_mode & S_IXUSR)) {
        DEBUG("CGI: Script not executable: " << requestPath.str());
        response.setStatusCode(HttpStatusCode::Forbidden);
        return false;
    }

    std::string extention = Utils::getFileExtension(requestPath.str());
    DEBUG("CGI: File extension: " << extention);

    if (!route.cgi_paths.exists(extention)) {
        DEBUG("CGI: No CGI interpreter found for extension: " << extention);
        response.setStatusCode(HttpStatusCode::NotImplemented);
        return false;
    }

    _interpreter = route.cgi_paths.getPath(extention);
    DEBUG("CGI: Using interpreter: " << _interpreter);

    struct stat interpreterStat;
    if (stat(_interpreter.c_str(), &interpreterStat) != 0) {
        DEBUG("CGI: Interpreter not found: " << _interpreter);
        response.setStatusCode(HttpStatusCode::InternalServerError);
        return false;
    }

    setupEnvironment(request, route, server, _interpreter);
    DEBUG("CGI: Environment variables set up");

    // Execute script
    std::string output, error;
    DEBUG("CGI: Executing script...");
    if (!executeScript(request.getBody(), output, error)) {
        ERROR("CGI execution failed: " << error);
        return false;
    }
    
    DEBUG("CGI: Script executed successfully, output length: " << output.length());
    
    // Parse output and build response
    return parseOutput(output, response);
}

void CGI::setupEnvironment(const Request& request, const LocationRule& route, const ServerConfig& server, const std::string &interpreterPath) {
    _env.clear();
    (void)route; // Unused parameter, but kept for consistency with the method signature
    // Standard CGI environment variables

    // std::string relativeScriptPath = "cgi-bin/test_cgi.php"; // or "test_cgi.php" depending on your routing
    std::string documentRoot = "/home/jbakker/Documents/programming/codam/webserv/";  // your actual server document root
// _scriptPath = documentRoot + "/" + interpreterPath; // e.g. "cgi-bin/test_cgi.php"
    Path scriptPathObj(documentRoot);
    scriptPathObj.append(Path::createFromUrl(request.metadata.getPath(), route).str());
    DEBUG("CGI: Script path: " << scriptPathObj.str());
    _scriptPath = scriptPathObj.str();

    Path interpreterPathObj(documentRoot);
    interpreterPathObj.append(interpreterPath);

    _env["SCRIPT_FILENAME"] = scriptPathObj.str();
    // _env["SCRIPT_FILENAME"] = interpreterPathObj.str().c_str();
    _env["REQUEST_METHOD"] = methodToStr(request.metadata.getMethod());
    _env["SCRIPT_NAME"] = request.metadata.getPath();
    // _env["SCRIPT_FILENAME"] = interpreterPathObj.str().c_str();
    _env["PATH_INFO"] = request.metadata.getPath();
    _env["PATH_TRANSLATED"] = interpreterPathObj.str().c_str();
    _env["REQUEST_URI"] = request.metadata.getPath();
    _env["SERVER_NAME"] = server.server_name.get().empty() ? "Whatever" : server.server_name.get();
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
    
    DEBUG("CGI Environment Variables set up");
    
    // Debug: print some key environment variables
    DEBUG("CGI: REQUEST_METHOD=" << _env["REQUEST_METHOD"]);
    DEBUG("CGI: SCRIPT_FILENAME=" << _env["SCRIPT_FILENAME"]);
    DEBUG("CGI: CONTENT_LENGTH=" << _env["CONTENT_LENGTH"]);
    DEBUG("CGI: REDIRECT_STATUS=" << _env["REDIRECT_STATUS"]);
}

bool CGI::executeScript(const std::string& input, std::string& output, std::string& error) {
    int inputPipe[2];   // Parent writes to child's stdin
    int outputPipe[2];  // Child writes to parent's stdout
    int errorPipe[2];   // Child writes to parent's stderr
    
    // Create pipes
    if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1 || pipe(errorPipe) == -1) {
        ERROR("Failed to create pipes: " << strerror(errno));
        return false;
    }
    
    DEBUG("CGI: About to fork...");
    
    pid_t pid = fork();
    if (pid == -1) {
        ERROR("Failed to fork: " << strerror(errno));
        close(inputPipe[0]); close(inputPipe[1]);
        close(outputPipe[0]); close(outputPipe[1]);
        close(errorPipe[0]); close(errorPipe[1]);
        return false;
    }
    
    if (pid == 0) {
        // Child process
        DEBUG("CGI Child: Starting child process");
        DEBUG("CGI Child: Script path: " << _scriptPath);
        DEBUG("CGI Child: Interpreter: " << _interpreter);
        
        // Close unused pipe ends
        close(inputPipe[1]);   // Close write end of input pipe
        close(outputPipe[0]);  // Close read end of output pipe
        close(errorPipe[0]);   // Close read end of error pipe
        
        // Redirect stdin, stdout, stderr
        if (dup2(inputPipe[0], STDIN_FILENO) == -1 ||
            dup2(outputPipe[1], STDOUT_FILENO) == -1 ||
            dup2(errorPipe[1], STDERR_FILENO) == -1) {
            // Can't use std::cerr here since stderr is redirected
            ERROR("Failed to redirect file descriptors: " << strerror(errno));
            exit(1);
        }
        
        // Close original pipe file descriptors
        close(inputPipe[0]);
        close(outputPipe[1]);
        close(errorPipe[1]);
        
        // Change to script directory
        std::string workDir = getWorkingDirectory(_scriptPath);
        if (chdir(workDir.c_str()) == -1) {
            write(STDERR_FILENO, "Failed to change directory\n", 27);
            exit(1);
        }
        
        // Set up environment
        std::vector<char*> envp;
        for (std::map<std::string, std::string>::const_iterator it = _env.begin(); 
             it != _env.end(); ++it) {
            std::string envVar = it->first + "=" + it->second;
            char* envStr = new char[envVar.length() + 1];
            strcpy(envStr, envVar.c_str());
            envp.push_back(envStr);
        }
        envp.push_back(NULL);
        
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
        
        // Check if script exists in working directory
        if (access(scriptName.c_str(), F_OK) != 0) {
            write(STDERR_FILENO, "Script not found in working directory\n", 38);
            exit(1);
        }
        
        if (access(scriptName.c_str(), X_OK) != 0) {
            write(STDERR_FILENO, "Script not executable in working directory\n", 43);
            exit(1);
        }
        
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
        close(inputPipe[0]);   // Close read end of input pipe
        close(outputPipe[1]);  // Close write end of output pipe
        close(errorPipe[1]);   // Close write end of error pipe
        
        // Set pipes to non-blocking
        fcntl(inputPipe[1], F_SETFL, O_NONBLOCK);
        fcntl(outputPipe[0], F_SETFL, O_NONBLOCK);
        fcntl(errorPipe[0], F_SETFL, O_NONBLOCK);
        
        // Write input to child's stdin
        if (!input.empty()) {
            size_t totalWritten = 0;
            while (totalWritten < input.length()) {
                ssize_t written = write(inputPipe[1], input.c_str() + totalWritten, 
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
        close(inputPipe[1]); // Close stdin to signal EOF
        
        // Read output and error
        output.clear();
        error.clear();
        
        fd_set readSet;
        struct timeval timeout;
        time_t startTime = time(NULL);
        bool processFinished = false;
        
        while (!processFinished) {
            // Check for timeout
            if (time(NULL) - startTime > TIMEOUT_SECONDS) {
                ERROR("CGI script timeout");
                kill(pid, SIGKILL);
                break;
            }
            
            // Check if process has finished
            int status;
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == pid) {
                processFinished = true;
                
                // Read any remaining output and error
                char buffer[4096];
                ssize_t bytesRead;
                
                // Read all remaining stdout
                while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0';
                    output.append(buffer, bytesRead);
                }
                
                // Read all remaining stderr
                while ((bytesRead = read(errorPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0';
                    error.append(buffer, bytesRead);
                }
                
                // Check exit status
                if (WIFEXITED(status)) {
                    int exitCode = WEXITSTATUS(status);
                    DEBUG("CGI: Process exited with code: " << exitCode);
                    if (exitCode != 0) {
                        ERROR("CGI script exited with code: " << exitCode);
                        if (!error.empty()) {
                            ERROR("CGI stderr: " << error);
                        }
                        close(outputPipe[0]);
                        close(errorPipe[0]);
                        return false;
                    }
                } else if (WIFSIGNALED(status)) {
                    int signal = WTERMSIG(status);
                    ERROR("CGI script terminated by signal: " << signal);
                    close(outputPipe[0]);
                    close(errorPipe[0]);
                    return false;
                }
                break;
            } else if (result == -1 && errno != ECHILD) {
                ERROR("waitpid error: " << strerror(errno));
                break;
            }
            
            FD_ZERO(&readSet);
            FD_SET(outputPipe[0], &readSet);
            FD_SET(errorPipe[0], &readSet);
            
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms
            
            int selectResult = select(std::max(outputPipe[0], errorPipe[0]) + 1, 
                                    &readSet, NULL, NULL, &timeout);
            
            if (selectResult == -1) {
                ERROR("Select error: " << strerror(errno));
                break;
            } else if (selectResult > 0) {
                // Read from stdout
                if (FD_ISSET(outputPipe[0], &readSet)) {
                    char buffer[4096];
                    ssize_t bytesRead = read(outputPipe[0], buffer, sizeof(buffer) - 1);
                    if (bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        output.append(buffer, bytesRead);
                        
                        // Check output size limit
                        if (output.length() > MAX_OUTPUT_SIZE) {
                            ERROR("CGI output too large");
                            kill(pid, SIGKILL);
                            break;
                        }
                    }
                }
                
                // Read from stderr
                if (FD_ISSET(errorPipe[0], &readSet)) {
                    char buffer[4096];
                    ssize_t bytesRead = read(errorPipe[0], buffer, sizeof(buffer) - 1);
                    if (bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        error.append(buffer, bytesRead);
                    }
                }
            }
        }
        
        // Close pipes
        close(outputPipe[0]);
        close(errorPipe[0]);
        
        DEBUG("CGI: Final output length: " << output.length());
        DEBUG("CGI: Final error length: " << error.length());
        if (!error.empty()) {
            DEBUG("CGI: Complete error output: " << error);
        }
        
        return true;
    }
}

bool CGI::parseOutput(const std::string& output, Response& response) {
    if (output.empty()) {
        ERROR("Empty CGI output");
        return false;
    }
    
    DEBUG("CGI: Parsing output (first 200 chars): " << output.substr(0, 200));
    
    // Find the header/body separator and its length
    size_t separatorPos = output.find("\r\n\r\n");
    size_t separatorLength = 4;
    
    if (separatorPos == std::string::npos) {
        separatorPos = output.find("\n\n");
        separatorLength = 2;
        
        if (separatorPos == std::string::npos) {
            // No headers found, treat entire output as body
            response.setStatusCode(HttpStatusCode::OK);
            response.setBody(output);
            return true;
        }
    }
    
    // Extract headers and body
    std::string headers = output.substr(0, separatorPos);
    std::string body = output.substr(separatorPos + separatorLength);
    
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
        
        DEBUG("Full header line: [" << line << "]");
        std::string rawValue = line.substr(colonPos + 1);
        DEBUG("Raw substring after colon: [" << rawValue << "]");
        std::string value = Utils::trim(rawValue);
        DEBUG("Trimmed value: [" << value << "]");

        std::string name = Utils::trim(line.substr(0, colonPos));
        
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
