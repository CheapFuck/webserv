#include "Client.hpp"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <fstream>      // for std::ifstream
#include <sstream>      // for std::ostringstream
#include <string>       // for std::string
#include <map>

Client::Client() : _socket(-1), _requestComplete(false), _responseComplete(false) {
    memset(&_addr, 0, sizeof(_addr));
}

Client::Client(int socket, const struct sockaddr_in& addr) 
    : _socket(socket), _addr(addr), _requestComplete(false), _responseComplete(false) {
}

Client::~Client() {
}



// Function to extract query parameters from a URL
std::map<std::string, std::string> parseQueryParams(const std::string& url) {
    std::map<std::string, std::string> params;
    size_t queryStart = url.find("?");
    
    if (queryStart == std::string::npos) {
        return params;  // No query string, return empty map
    }

    std::string queryString = url.substr(queryStart + 1);  // Skip past "?"
    std::istringstream queryStream(queryString);
    std::string param;

    while (std::getline(queryStream, param, '&')) {
        size_t eqPos = param.find("=");
        if (eqPos != std::string::npos) {
            std::string key = param.substr(0, eqPos);
            std::string value = param.substr(eqPos + 1);
            params[key] = value;
        }
    }

    return params;
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
    // Find server configuration
    const ServerConfig* server = config.findServer("0.0.0.0", 8080, _request.getHeader("Host"));
    if (!server) {
        _response.setStatusCode(404);
        _response.setBody("<html><body><h1>404 Not Found</h1><p>Server not found</p></body></html>");
        return;
    }
    
    // Find route configuration
    const RouteConfig* route = config.findRoute(*server, _request.getPath());
    if (!route) {
        _response.setStatusCode(404);
        _response.setBody("<html><body><h1>404 Not Found</h1><p>Route not found</p></body></html>");
        return;
    }
    
    // Check if method is allowed
    if (!route->methods.empty()) {
        bool methodAllowed = false;
        for (size_t i = 0; i < route->methods.size(); ++i) {
            if (route->methods[i] == _request.getMethod()) {
                methodAllowed = true;
                break;
            }
        }
        std::cout << "Request received: " << _request.getMethod() << " " << _request.getPath() << std::endl;
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
    
    // Handle request based on method
    // if (_request.getMethod() == "GET") {
    //     // TODO: Implement file serving
    //     _response.setStatusCode(200);
    //     _response.setBody("<html><body><h1>Hello, World!</h1><p>This is a placeholder response.</p></body></html>");
    // } else if (_request.getMethod() == "POST") {
    //     // TODO: Implement file upload and form handling
    //     _response.setStatusCode(200);
    //     _response.setBody("<html><body><h1>POST Request Received</h1><p>This is a placeholder response.</p></body></html>");
    // } else if (_request.getMethod() == "DELETE") {
    //     // TODO: Implement file deletion
    //     _response.setStatusCode(200);
    //     _response.setBody("<html><body><h1>DELETE Request Received</h1><p>This is a placeholder response.</p></body></html>");
    // } else {
    //     _response.setStatusCode(501);
    //     _response.setBody("<html><body><h1>501 Not Implemented</h1><p>Method not implemented</p></body></html>");
    // }

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
    }
// Assuming you're using your own HTTP server implementation
else if (_request.getMethod() == "POST" && _request.getPath() == "/upload") {
    std::string contentType = _request.getHeader("Content-Type");

    if (contentType.find("multipart/form-data") != std::string::npos) {
        std::string boundary = extractBoundary(contentType);
        std::string body = _request.getBody();

        std::map<std::string, std::string> formFields;
        std::vector<FilePart> uploadedFiles;

        parseMultipartFormData(body, boundary, formFields, uploadedFiles);

        // Save uploaded files (e.g., to the "uploads" directory)
        for (const auto& filePart : uploadedFiles) {
            std::ofstream outFile("./uploads/" + filePart.filename, std::ios::binary);
            outFile.write(filePart.content.c_str(), filePart.content.size());
            outFile.close();
        }

        _response.setStatusCode(200);
        _response.setBody("<html><body><h1>Upload Complete</h1></body></html>");
    } else {
        _response.setStatusCode(415); // Unsupported Media Type
        _response.setBody("<html><body><h1>415 Unsupported Media Type</h1></body></html>");
    }

// // Example route definition
// RouteConfig uploadRoute;
// uploadRoute.path = "/upload";
// uploadRoute.methods = {"POST"};  // Allow only POST method
}

    
    else if (_request.getMethod() == "DELETE") {
        std::string url = _request.getPath();  // Get the full request URL
        std::map<std::string, std::string> queryParams = parseQueryParams(url);
    
        // Ensure the file parameter is present
        if (queryParams.find("file") == queryParams.end()) {
            _response.setStatusCode(400);
            _response.setBody("<html><body><h1>400 Bad Request: File parameter missing</h1></body></html>");
            return;
        }
    
        std::string filePath = queryParams["file"];  // Get the file parameter
    
        // Ensure the file path is safe
        if (filePath.find("..") != std::string::npos || filePath.find("/uploads/") != 0) {
            _response.setStatusCode(400);
            _response.setBody("<html><body><h1>400 Bad Request: Invalid file path</h1></body></html>");
            return;
        }
    
        std::string fullPath = "./uploads/" + filePath;
        if (std::remove(fullPath.c_str()) == 0) {
            _response.setStatusCode(200);
            _response.setBody("<html><body><h1>File Deleted</h1></body></html>");
        } else {
            _response.setStatusCode(404);
            _response.setBody("<html><body><h1>404 File Not Found</h1></body></html>");
        }
    }
    
    
    // if (!route->methods.empty()) {
    //     bool methodAllowed = false;
    //     for (size_t i = 0; i < route->methods.size(); ++i) {
    //         if (route->methods[i] == _request.getMethod()) {
    //             methodAllowed = true;
    //             break;
    //         }
    //     }
        
    //     if (!methodAllowed) {
    //         _response.setStatusCode(405);
    //         _response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
    //         return;
    //     }
    // }
    
    
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
