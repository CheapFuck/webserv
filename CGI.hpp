#pragma once

#include "config/rules/rules.hpp"
#include "response.hpp"
#include "request.hpp"

#include <vector>
#include <string>
#include <map>

class CGI {
private:
    static const int TIMEOUT_SECONDS = 30;
    static const size_t MAX_OUTPUT_SIZE = 10 * 1024 * 1024; // 10MB
    
    std::map<std::string, std::string> _env;
    std::string _scriptPath;
    std::string _interpreter;
    
    // Private methods
    void setupEnvironment(const Request& request, const LocationRule& route, const ServerConfig& server, const std::string &interpreterPath);
    bool executeScript(const std::string& input, std::string& output, std::string& error);
    bool parseOutput(const std::string& output, Response& response);
    std::string getWorkingDirectory(const std::string& scriptPath);
    
public:
    CGI();
    ~CGI();
    
    bool execute(const Request& request, const LocationRule& route, const ServerConfig& server, Response& response);
};
