#pragma once

#include "config/rules/rules.hpp"
#include "response.hpp"
#include "request.hpp"

#include <vector>
#include <string>
#include <map>
#include <future>

class CGI {
public:
    enum class Status {
        RUNNING,
        FINISHED,
        ERROR,
        TIMEOUT
    };

    CGI();
    ~CGI();
    
    // Start CGI execution asynchronously
    bool startExecution(const Request& request, const LocationRule& route, const ServerConfig& server);
    
    // Update execution status
    Status updateExecution();
    
    // Get the response after execution is complete
    bool getResponse(Response& response);
    
    // Get file descriptors for epoll monitoring
    std::vector<int> getFileDescriptors() const;

private:
    static const int TIMEOUT_SECONDS = 30;
    static const size_t MAX_OUTPUT_SIZE = 10 * 1024 * 1024; // 10MB
    
    std::map<std::string, std::string> _env;
    std::string _scriptPath;
    std::string _interpreter;
    
    // Pipe file descriptors
    int _inputPipe[2];   // Parent writes to child's stdin
    int _outputPipe[2];  // Child writes to parent's stdout
    int _errorPipe[2];   // Child writes to parent's stderr
    
    // Process ID of the CGI script
    pid_t _pid;
    
    // Execution state
    Status _status;
    time_t _startTime;
    
    // Output buffers
    std::string _output;
    std::string _error;
    
    // Future for async execution
    std::future<bool> _executionFuture;
    
    // Private methods
    void setupEnvironment(const Request& request, const LocationRule& route, const ServerConfig& server, const std::string &interpreterPath);
    bool executeScript(const std::string& input);
    bool parseOutput(Response& response);
    std::string getWorkingDirectory(const std::string& scriptPath);
    
    // Read from pipes
    bool readFromPipes();
    
    // Check if process has finished
    bool checkProcessStatus();
};