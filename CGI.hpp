#ifndef CGI_HPP
#define CGI_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"
#include <string>
#include <map>
#include <vector>

class CGI {
private:
    static const int TIMEOUT_SECONDS = 30;
    static const size_t MAX_OUTPUT_SIZE = 10 * 1024 * 1024; // 10MB
    
    std::map<std::string, std::string> _env;
    std::string _scriptPath;
    std::string _interpreter;
    
    // Private methods
    void setupEnvironment(const Request& request, const RouteConfig& route, const ServerConfig& server);
    bool executeScript(const std::string& input, std::string& output, std::string& error);
    bool parseOutput(const std::string& output, Response& response);
    std::string getWorkingDirectory(const std::string& scriptPath);
    
public:
    CGI();
    ~CGI();
    
    bool execute(const Request& request, const RouteConfig& route, const ServerConfig& server, Response& response);
};

#endif // CGI_HPP
