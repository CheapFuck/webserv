#include "CGI.hpp"
#include "Utils.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>

CGI::CGI() {
}

CGI::~CGI() {
}

bool CGI::execute(const Request& request, const RouteConfig& route, const ServerConfig& server, Response& response) {
    // Determine script path
    std::string requestPath = request.getPath();
    if (requestPath.find(route.path) == 0) {
        _scriptPath = route.root + requestPath.substr(route.path.length());
    } else {
        _scriptPath = route.root + requestPath;
    }
    
    // Check if script exists and is executable
    struct stat statBuf;
    if (stat(_scriptPath.c_str(), &statBuf) != 0) {
        std::cerr << "CGI script not found: " << _scriptPath << std::endl;
        return false;
    }
    
    if (!(statBuf.st_mode & S_IXUSR)) {
        std::cerr << "CGI script not executable: " << _scriptPath << std::endl;
        return false;
    }
    
    // Get file extension and find interpreter
    std::string extension = Utils::getFileExtension(_scriptPath);
    std::map<std::string, std::string>::const_iterator it = route.cgiExtensions.find(extension);
    if (it == route.cgiExtensions.end()) {
        std::cerr << "No CGI interpreter found for extension: " << extension << std::endl;
        return false;
    }
    _interpreter = it->second;
    
    // Setup environment variables
    setupEnvironment(request, route, server);
    
    // Execute script
    std::string output, error;
    if (!executeScript(request.getBody(), output, error)) {
        std::cerr << "CGI execution failed: " << error << std::endl;
        return false;
    }
    
    // Parse output and build response
    return parseOutput(output, response);
}

void CGI::setupEnvironment(const Request& request, const RouteConfig& route, const ServerConfig& server) {
    _env.clear();
    (void)route; // Unused parameter, can be removed if not needed
    // Standard CGI environment variables
    _env["REQUEST_METHOD"] = request.getMethod();
    _env["SCRIPT_NAME"] = request.getPath();
    _env["SCRIPT_FILENAME"] = _scriptPath;
    _env["PATH_INFO"] = request.getPath();
    _env["PATH_TRANSLATED"] = _scriptPath;
    _env["REQUEST_URI"] = request.getPath();
    _env["SERVER_NAME"] = server.serverNames.empty() ? server.host : server.serverNames[0];
    _env["SERVER_PORT"] = Utils::intToString(server.port);
    _env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _env["SERVER_SOFTWARE"] = "webserv/1.0";
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    
    // Query string
    size_t queryPos = request.getPath().find('?');
    if (queryPos != std::string::npos) {
        _env["QUERY_STRING"] = request.getPath().substr(queryPos + 1);
        _env["SCRIPT_NAME"] = request.getPath().substr(0, queryPos);
    } else {
        _env["QUERY_STRING"] = "";
    }
    
    // Content length and type for POST requests
    if (request.getMethod() == "POST") {
        _env["CONTENT_LENGTH"] = Utils::intToString(request.getBody().length());
        std::string contentType = request.getHeader("Content-Type");
        if (!contentType.empty()) {
            _env["CONTENT_TYPE"] = contentType;
        }
    }
    
    // HTTP headers as environment variables
    std::string host = request.getHeader("Host");
    if (!host.empty()) {
        _env["HTTP_HOST"] = host;
    }
    
    std::string userAgent = request.getHeader("User-Agent");
    if (!userAgent.empty()) {
        _env["HTTP_USER_AGENT"] = userAgent;
    }
    
    std::string accept = request.getHeader("Accept");
    if (!accept.empty()) {
        _env["HTTP_ACCEPT"] = accept;
    }
    
    std::string acceptLanguage = request.getHeader("Accept-Language");
    if (!acceptLanguage.empty()) {
        _env["HTTP_ACCEPT_LANGUAGE"] = acceptLanguage;
    }
    
    std::string acceptEncoding = request.getHeader("Accept-Encoding");
    if (!acceptEncoding.empty()) {
        _env["HTTP_ACCEPT_ENCODING"] = acceptEncoding;
    }
    
    std::string cookie = request.getHeader("Cookie");
    if (!cookie.empty()) {
        _env["HTTP_COOKIE"] = cookie;
    }
    
    std::string referer = request.getHeader("Referer");
    if (!referer.empty()) {
        _env["HTTP_REFERER"] = referer;
    }
    
    // Remote address (simplified - you might want to get actual client IP)
    _env["REMOTE_ADDR"] = "127.0.0.1";
    _env["REMOTE_HOST"] = "localhost";
    
    // Working directory
    _env["PWD"] = getWorkingDirectory(_scriptPath);
}

bool CGI::executeScript(const std::string& input, std::string& output, std::string& error) {
    int inputPipe[2];   // Parent writes to child's stdin
    int outputPipe[2];  // Child writes to parent's stdout
    int errorPipe[2];   // Child writes to parent's stderr
    
    // Create pipes
    if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1 || pipe(errorPipe) == -1) {
        std::cerr << "Failed to create pipes: " << strerror(errno) << std::endl;
        return false;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork: " << strerror(errno) << std::endl;
        close(inputPipe[0]); close(inputPipe[1]);
        close(outputPipe[0]); close(outputPipe[1]);
        close(errorPipe[0]); close(errorPipe[1]);
        return false;
    }
    
    if (pid == 0) {
        // Child process
        
        // Close unused pipe ends
        close(inputPipe[1]);   // Close write end of input pipe
        close(outputPipe[0]);  // Close read end of output pipe
        close(errorPipe[0]);   // Close read end of error pipe
        
        // Redirect stdin, stdout, stderr
        if (dup2(inputPipe[0], STDIN_FILENO) == -1 ||
            dup2(outputPipe[1], STDOUT_FILENO) == -1 ||
            dup2(errorPipe[1], STDERR_FILENO) == -1) {
            std::cerr << "Failed to redirect file descriptors: " << strerror(errno) << std::endl;
            exit(1);
        }
        
        // Close original pipe file descriptors
        close(inputPipe[0]);
        close(outputPipe[1]);
        close(errorPipe[1]);
        
        // Change to script directory
        std::string workDir = getWorkingDirectory(_scriptPath);
        if (chdir(workDir.c_str()) == -1) {
            std::cerr << "Failed to change directory to: " << workDir << std::endl;
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
        
        // Add script path
        char* scriptStr = new char[_scriptPath.length() + 1];
        strcpy(scriptStr, _scriptPath.c_str());
        argv.push_back(scriptStr);
        
        argv.push_back(NULL);
        
        // Execute script
        execve(_interpreter.c_str(), &argv[0], &envp[0]);
        
        // If we reach here, execve failed
        std::cerr << "Failed to execute CGI script: " << strerror(errno) << std::endl;
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
                        std::cerr << "Failed to write to CGI stdin: " << strerror(errno) << std::endl;
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
        
        while (true) {
            // Check for timeout
            if (time(NULL) - startTime > TIMEOUT_SECONDS) {
                std::cerr << "CGI script timeout" << std::endl;
                kill(pid, SIGKILL);
                break;
            }
            
            FD_ZERO(&readSet);
            FD_SET(outputPipe[0], &readSet);
            FD_SET(errorPipe[0], &readSet);
            
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            int selectResult = select(std::max(outputPipe[0], errorPipe[0]) + 1, 
                                    &readSet, NULL, NULL, &timeout);
            
            if (selectResult == -1) {
                std::cerr << "Select error: " << strerror(errno) << std::endl;
                break;
            } else if (selectResult == 0) {
                // Timeout, continue loop to check overall timeout
                continue;
            }
            
            // Read from stdout
            if (FD_ISSET(outputPipe[0], &readSet)) {
                char buffer[4096];
                ssize_t bytesRead = read(outputPipe[0], buffer, sizeof(buffer) - 1);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    output.append(buffer, bytesRead);
                    
                    // Check output size limit
                    if (output.length() > MAX_OUTPUT_SIZE) {
                        std::cerr << "CGI output too large" << std::endl;
                        kill(pid, SIGKILL);
                        break;
                    }
                } else if (bytesRead == 0) {
                    // EOF on stdout
                    break;
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
        
        // Close pipes
        close(outputPipe[0]);
        close(errorPipe[0]);
        
        // Wait for child process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "Failed to wait for child process: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Check exit status
        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            if (exitCode != 0) {
                std::cerr << "CGI script exited with code: " << exitCode << std::endl;
                return false;
            }
        } else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            std::cerr << "CGI script terminated by signal: " << signal << std::endl;
            return false;
        }
        
        return true;
    }
}

bool CGI::parseOutput(const std::string& output, Response& response) {
    if (output.empty()) {
        std::cerr << "Empty CGI output" << std::endl;
        return false;
    }
    
    // Find the end of headers
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        if (headerEnd == std::string::npos) {
            // No headers, treat entire output as body
            response.setStatusCode(200);
            response.setBody(output);
            return true;
        }
        headerEnd += 2; // For \n\n
    } else {
        headerEnd += 4; // For \r\n\r\n
    }
    
    // Parse headers
    std::string headers = output.substr(0, headerEnd - (headerEnd > 2 ? 4 : 2));
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
            std::cerr << "Invalid CGI header: " << line << std::endl;
            continue;
        }
        
        std::string name = Utils::trim(line.substr(0, colonPos));
        std::string value = Utils::trim(line.substr(colonPos + 1));
        
        // Handle special headers
        if (Utils::toLower(name) == "status") {
            int statusCode = Utils::stringToInt(value.substr(0, 3));
            response.setStatusCode(statusCode);
            statusSet = true;
        } else if (Utils::toLower(name) == "location") {
            response.setHeader("Location", value);
            if (!statusSet) {
                response.setStatusCode(302); // Default redirect status
            }
        } else {
            response.setHeader(name, value);
        }
    }
    
    // Set default status if not set
    if (!statusSet) {
        response.setStatusCode(200);
    }
    
    // Set body
    if (headerEnd < output.length()) {
        response.setBody(output.substr(headerEnd));
    }
    
    return true;
}

std::string CGI::getWorkingDirectory(const std::string& scriptPath) {
    size_t lastSlash = scriptPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        return scriptPath.substr(0, lastSlash);
    }
    return ".";
}
