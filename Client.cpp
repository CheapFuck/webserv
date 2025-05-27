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

struct FilePart {
    std::string name;
    std::string filename;
    std::string contentType;
    std::string content;
};


// Extracts boundary string from Content-Type
std::string extractBoundary(const std::string& contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos != std::string::npos) {
        return "--" + contentType.substr(pos + 9); // skip "boundary="
    }
    return "";
}
// Utility: extract string between two markers
std::string extractBetween(const std::string& str, const std::string& start, const std::string& end) {
    size_t s = str.find(start);
    if (s == std::string::npos) return "";
    s += start.length();
    size_t e = str.find(end, s);
    return (e == std::string::npos) ? "" : str.substr(s, e - s);
}


// Crude multipart/form-data parser (simplified!)
void parseMultipartFormData(const std::string& body, const std::string& boundary,
                            std::map<std::string, std::string>& formFields,
                            std::vector<FilePart>& files) {
    size_t pos = 0, next;
    while ((next = body.find(boundary, pos)) != std::string::npos) {
        size_t headerEnd = body.find("\r\n\r\n", pos);
        if (headerEnd == std::string::npos) break;

        std::string headers = body.substr(pos, headerEnd - pos);
        size_t contentStart = headerEnd + 4;
        size_t contentEnd = body.find(boundary, contentStart) - 2; // exclude \r\n

        std::string content = body.substr(contentStart, contentEnd - contentStart);
        FilePart part;

        if (headers.find("filename=\"") != std::string::npos) {
            part.name = extractBetween(headers, "name=\"", "\"");
            part.filename = extractBetween(headers, "filename=\"", "\"");
            part.content = content;
            files.push_back(part);
        } else {
            std::string name = extractBetween(headers, "name=\"", "\"");
            formFields[name] = content;
        }

        pos = contentEnd + boundary.length() + 2; // move to next part
    }
    
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
        std::string rootDir = "./"; // Set this to your static content root
        std::string requestPath = _request.getPath(); // e.g., "/index.html"
        
        // Prevent directory traversal (like "/../../etc/passwd")
        if (requestPath.find("..") != std::string::npos) {
            _response.setStatusCode(400);
            _response.setBody("<html><body><h1>400 Bad Request</h1><p>Invalid file path</p></body></html>");
            return;
        }
    
        // Default file fallback
        if (requestPath == "/") requestPath = "/index.html";
    
        std::string fullPath = rootDir + requestPath;
    
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            _response.setStatusCode(404);
            _response.setBody("<html><body><h1>404 Not Found</h1><p>File not found</p></body></html>");
        } else {
            std::ostringstream ss;
            ss << file.rdbuf();
            file.close();
    
            _response.setStatusCode(200);
            _response.setHeader("Content-Type", getMimeType(fullPath)); // We'll define this next
            _response.setBody(ss.str());
        }
    }else if (_request.getMethod() == "POST" && _request.getPath() == "/upload") {
        std::string contentType = _request.getHeader("Content-Type");
    
        if (contentType.find("multipart/form-data") != std::string::npos) {
            std::cout << "[DEBUG] Content-Type indicates multipart/form-data\n";
        
            std::string boundary = extractBoundary(contentType);
            std::cout << "[DEBUG] Extracted boundary: " << boundary << "\n";
        
            std::string body = _request.getBody();
            std::cout << "[DEBUG] Request body length: " << body.size() << "\n";
        
            std::map<std::string, std::string> formFields;
            std::vector<FilePart> uploadedFiles;
        
            parseMultipartFormData(body, boundary, formFields, uploadedFiles);
            std::cout << "[DEBUG] Parsed form fields count: " << formFields.size() << "\n";
            std::cout << "[DEBUG] Parsed uploaded files count: " << uploadedFiles.size() << "\n";
        
            // Save uploaded files (e.g., to the "uploads" directory)
            for (const auto& filePart : uploadedFiles) {
                std::cout << "[DEBUG] Saving file: " << filePart.filename << " (size: " << filePart.content.size() << " bytes)\n";
        
                std::ofstream outFile("./uploads/" + filePart.filename, std::ios::binary);
                if (!outFile) {
                    std::cerr << "[ERROR] Failed to open file for writing: " << filePart.filename << "\n";
                    continue;
                }
                outFile.write(filePart.content.c_str(), filePart.content.size());
                outFile.close();
        
                std::cout << "[DEBUG] Successfully saved file: " << filePart.filename << "\n";
            }
        }
        
            _response.setStatusCode(200);
            _response.setBody("<html><body><h1>Upload Complete</h1></body></html>");
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
