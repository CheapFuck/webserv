#include "Client.hpp"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <fstream>      // for std::ifstream
#include <sstream>      // for std::ostringstream
#include <string>       // for std::string
#include <map>
#include "CGI.hpp"
#include "Utils.hpp"

Client::Client() : _socket(-1), _requestComplete(false), _responseComplete(false) {
    memset(&_addr, 0, sizeof(_addr));
}

Client::Client(int socket, const struct sockaddr_in& addr) 
    : _socket(socket), _addr(addr), _requestComplete(false), _responseComplete(false) {
}

Client::~Client() {
}

bool Client::readRequest() {
    char buf[4096];
    ssize_t bytesRead = recv(_socket, buf, sizeof(buf) - 1, 0);
    
    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            // Client closed connection
            return false;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, not an error
            return true;
        } else {
            // Error
            std::cerr << "Error reading from client: " << strerror(errno) << std::endl;
            return false;
        }
    }
    
    // Append data to buffer
    buf[bytesRead] = '\0';
    _buffer.append(buf, bytesRead);
    
    // Try to parse request
    if (_request.parse(_buffer)) {
        _requestComplete = true;
    }
    
    return true;
}

bool Client::isRequestComplete() const {
    return _requestComplete;
}

void Client::processRequest(const Config& config) {
    std::cout << "Processing request: " << _request.getMethod() << " " << _request.getPath() << std::endl;
    
    // Find server configuration
    const ServerConfig* server = config.findServer("0.0.0.0", 8080, _request.getHeader("Host"));
    if (!server) {
        std::cout << "No server found for request" << std::endl;
        _response.setStatusCode(404);
        _response.setBody("<html><body><h1>404 Not Found</h1><p>Server not found</p></body></html>");
        return;
    }
    
    std::cout << "Found server config" << std::endl;
    
    // Find route configuration
    const RouteConfig* route = config.findRoute(*server, _request.getPath());
    if (!route) {
        std::cout << "No route found for path: " << _request.getPath() << std::endl;
        _response.setStatusCode(404);
        _response.setBody("<html><body><h1>404 Not Found</h1><p>Route not found</p></body></html>");
        return;
    }
    
    std::cout << "Found route: " << route->path << " (root: " << route->root << ")" << std::endl;
    
    // Check if method is allowed
    if (!route->methods.empty()) {
        bool methodAllowed = false;
        for (size_t i = 0; i < route->methods.size(); ++i) {
            if (route->methods[i] == _request.getMethod()) {
                methodAllowed = true;
                break;
            }
        }
        
        if (!methodAllowed) {
            _response.setStatusCode(405);
            _response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
            return;
        }
    }
    
    // Handle redirect
    if (!route->redirect.empty()) {
        _response.setStatusCode(301);
        _response.setHeader("Location", route->redirect);
        return;
    }

    // Check if this is a CGI request
    std::string requestPath = _request.getPath();
    std::string extension = Utils::getFileExtension(requestPath);
    
    std::cout << "Request path: " << requestPath << std::endl;
    std::cout << "File extension: " << extension << std::endl;
    std::cout << "Available CGI extensions: ";
    for (std::map<std::string, std::string>::const_iterator it = route->cgiExtensions.begin();
         it != route->cgiExtensions.end(); ++it) {
        std::cout << it->first << " ";
    }
    std::cout << std::endl;

    if (!route->cgiExtensions.empty() && route->cgiExtensions.find(extension) != route->cgiExtensions.end()) {
        std::cout << "This is a CGI request!" << std::endl;
        // This is a CGI request
        CGI cgi;
        if (cgi.execute(_request, *route, *server, _response)) {
            std::cout << "CGI execution successful" << std::endl;
            return; // CGI handled the response
        } else {
            std::cout << "CGI execution failed" << std::endl;
            _response.setStatusCode(500);
            _response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI execution failed</p></body></html>");
            return;
        }
    }
    
    std::cout << "Not a CGI request, handling as static content" << std::endl;
    
    // Handle request based on method
    if (_request.getMethod() == "GET") {
        // For now, let's serve the file content as-is (this is what's happening)
        std::string filePath = route->root + _request.getPath().substr(route->path.length());
        std::cout << "Trying to serve static file: " << filePath << std::endl;
        
        // TODO: Implement proper file serving
        _response.setStatusCode(200);
        _response.setHeader("Content-Type", "application/octet-stream");
        
        // Read file content (simplified)
        std::ifstream file(filePath.c_str());
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            _response.setBody(content);
            file.close();
        } else {
            _response.setStatusCode(404);
            _response.setBody("<html><body><h1>404 Not Found</h1><p>File not found</p></body></html>");
        }
    } else if (_request.getMethod() == "POST") {
        // TODO: Implement file upload and form handling
        _response.setStatusCode(200);
        _response.setBody("<html><body><h1>POST Request Received</h1><p>This is a placeholder response.</p></body></html>");
    } else if (_request.getMethod() == "DELETE") {
        // TODO: Implement file deletion
        _response.setStatusCode(200);
        _response.setBody("<html><body><h1>DELETE Request Received</h1><p>This is a placeholder response.</p></body></html>");
    } else {
        _response.setStatusCode(501);
        _response.setBody("<html><body><h1>501 Not Implemented</h1><p>Method not implemented</p></body></html>");
    }
}

bool Client::sendResponse() {
    std::string responseStr = _response.toString();
    
    if (responseStr.empty()) {
        _responseComplete = true;
        return true;
    }
    
    ssize_t bytesSent = send(_socket, responseStr.c_str(), responseStr.length(), 0);
    
    if (bytesSent <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Would block, try again later
            return true;
        } else {
            // Error
            std::cerr << "Error sending to client: " << strerror(errno) << std::endl;
            return false;
        }
    }
    
    // Remove sent data from response
    _response.dataSent(bytesSent);
    
    // Check if response is complete
    if (_response.isEmpty()) {
        _responseComplete = true;
    }
    
    return true;
}

bool Client::isResponseComplete() const {
    return _responseComplete;
}

void Client::reset() {
    _buffer.clear();
    _request = Request();
    _response = Response();
    _requestComplete = false;
    _responseComplete = false;
}
