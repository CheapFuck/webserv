#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct RouteConfig {
    std::string path;
    std::vector<std::string> methods;
    std::string root;
    std::string index;
    bool autoindex;
    std::string redirect;
    std::string uploadDir;
    std::map<std::string, std::string> cgiExtensions;
    
    RouteConfig() : autoindex(false) {}
};

struct ServerConfig {
    std::string host;
    int port;
    std::vector<std::string> serverNames;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::vector<RouteConfig> routes;
    
    ServerConfig() : port(80), clientMaxBodySize(1024 * 1024) {} // Default 1MB
};

class Config {
private:
    std::vector<ServerConfig> _servers;
    
    // Private methods for parsing
    bool parseServer(std::ifstream& file, ServerConfig& server);
    bool parseRoute(std::ifstream& file, RouteConfig& route);
    
public:
    Config();
    ~Config();
    
    bool parseFile(const std::string& path);
    const std::vector<ServerConfig>& getServers() const;
    
    // Helper methods to find server and route configurations
    const ServerConfig* findServer(const std::string& host, int port, const std::string& serverName) const;
    const RouteConfig* findRoute(const ServerConfig& server, const std::string& path) const;
};

#endif // CONFIG_HPP
