#include "Config.hpp"
#include "Utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

Config::Config() {
}

Config::~Config() {
}

bool Config::parseFile(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        std::cerr << "Failed to open configuration file: " << path << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        if (line == "server {") {
            ServerConfig server;
            if (!parseServer(file, server)) {
                return false;
            }
            _servers.push_back(server);
        } else {
            std::cerr << "Unexpected line in config file: " << line << std::endl;
            return false;
        }
    }
    
    if (_servers.empty()) {
        std::cerr << "No server configurations found" << std::endl;
        return false;
    }
    
    return true;
}

bool Config::parseServer(std::ifstream& file, ServerConfig& server) {
    std::string line;
    while (std::getline(file, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        if (line == "}") {
            return true;
        } else if (Utils::startsWith(line, "listen")) {
            std::string hostPort = Utils::trim(line.substr(7, line.length() - 8)); // Remove "listen" and ";"
            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos) {
                server.host = hostPort.substr(0, colonPos);
                server.port = Utils::stringToInt(hostPort.substr(colonPos + 1));
            } else {
                server.host = "0.0.0.0";
                server.port = Utils::stringToInt(hostPort);
            }
        } else if (Utils::startsWith(line, "server_name")) {
            std::string names = Utils::trim(line.substr(12, line.length() - 13)); // Remove "server_name" and ";"
            server.serverNames = Utils::split(names, ' ');
        } else if (Utils::startsWith(line, "error_page")) {
            std::string errorConfig = Utils::trim(line.substr(11, line.length() - 12)); // Remove "error_page" and ";"
            std::vector<std::string> parts = Utils::split(errorConfig, ' ');
            if (parts.size() >= 2) {
                std::string page = parts.back();
                for (size_t i = 0; i < parts.size() - 1; ++i) {
                    int code = Utils::stringToInt(parts[i]);
                    server.errorPages[code] = page;
                }
            }
        } else if (Utils::startsWith(line, "client_max_body_size")) {
            std::string sizeStr = Utils::trim(line.substr(21, line.length() - 22)); // Remove "client_max_body_size" and ";"
            server.clientMaxBodySize = Utils::parseSize(sizeStr);
        } else if (Utils::startsWith(line, "location")) {
            size_t pathStart = line.find(' ') + 1;
            size_t pathEnd = line.find(' ', pathStart);
            std::string path = line.substr(pathStart, pathEnd - pathStart);
            
            if (line[line.length() - 1] != '{') {
                std::cerr << "Expected '{' after location path" << std::endl;
                return false;
            }
            
            RouteConfig route;
            route.path = path;
            if (!parseRoute(file, route)) {
                return false;
            }
            server.routes.push_back(route);
        } else {
            std::cerr << "Unknown directive in server block: " << line << std::endl;
            return false;
        }
    }
    
    std::cerr << "Unexpected end of file while parsing server block" << std::endl;
    return false;
}

bool Config::parseRoute(std::ifstream& file, RouteConfig& route) {
    std::string line;
    while (std::getline(file, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        if (line == "}") {
            return true;
        } else if (Utils::startsWith(line, "root")) {
            route.root = Utils::trim(line.substr(5, line.length() - 6)); // Remove "root" and ";"
        } else if (Utils::startsWith(line, "index")) {
            route.index = Utils::trim(line.substr(6, line.length() - 7)); // Remove "index" and ";"
        } else if (Utils::startsWith(line, "autoindex")) {
            std::string value = Utils::trim(line.substr(10, line.length() - 11)); // Remove "autoindex" and ";"
            route.autoindex = (value == "on");
        } else if (Utils::startsWith(line, "return")) {
            route.redirect = Utils::trim(line.substr(7, line.length() - 8)); // Remove "return" and ";"
        } else if (Utils::startsWith(line, "upload_store")) {
            route.uploadDir = Utils::trim(line.substr(13, line.length() - 14)); // Remove "upload_store" and ";"
        } else if (Utils::startsWith(line, "allowed_methods")) {
            std::string methods = Utils::trim(line.substr(16, line.length() - 17)); // Remove "allowed_methods" and ";"
            route.methods = Utils::split(methods, ' ');
        } else if (Utils::startsWith(line, "cgi_pass")) {
            std::string cgiConfig = Utils::trim(line.substr(9, line.length() - 10)); // Remove "cgi_pass" and ";"
            std::vector<std::string> parts = Utils::split(cgiConfig, ' ');
            if (parts.size() == 2) {
                route.cgiExtensions[parts[0]] = parts[1];
            }
        } else {
            std::cerr << "Unknown directive in location block: " << line << std::endl;
            return false;
        }
    }
    
    std::cerr << "Unexpected end of file while parsing location block" << std::endl;
    return false;
}

const std::vector<ServerConfig>& Config::getServers() const {
    return _servers;
}

const ServerConfig* Config::findServer(const std::string& host, int port, const std::string& serverName) const {
    const ServerConfig* defaultServer = NULL;
    
    for (size_t i = 0; i < _servers.size(); ++i) {
        const ServerConfig& server = _servers[i];
        
        // Check if server matches host and port
        if (server.host == host && server.port == port) {
            // Check if server_name matches
            if (!serverName.empty()) {
                for (size_t j = 0; j < server.serverNames.size(); ++j) {
                    if (server.serverNames[j] == serverName) {
                        return &server;
                    }
                }
            }
            
            // If no default server yet, use this one
            if (!defaultServer) {
                defaultServer = &server;
            }
        }
    }
    
    return defaultServer;
}

const RouteConfig* Config::findRoute(const ServerConfig& server, const std::string& path) const {
    const RouteConfig* bestMatch = NULL;
    size_t bestMatchLength = 0;
    
    for (size_t i = 0; i < server.routes.size(); ++i) {
        const RouteConfig& route = server.routes[i];
        
        // Check if route path is a prefix of the requested path
        if (path.find(route.path) == 0) {
            // If this is a longer match than the current best, use it
            if (route.path.length() > bestMatchLength) {
                bestMatch = &route;
                bestMatchLength = route.path.length();
            }
        }
    }
    
    return bestMatch;
}
