#include "Response.hpp"
#include <sstream>
#include <ctime>

Response::Response() : _statusCode(200), _built(false) {
}

Response::~Response() {
}

void Response::setStatusCode(int code) {
    _statusCode = code;
    _built = false;
}

void Response::setHeader(const std::string& name, const std::string& value) {
    _headers[name] = value;
    _built = false;
}

void Response::setBody(const std::string& body) {
    _body = body;
    _built = false;
}

std::string Response::toString() {
    if (!_built) {
        build();
    }
    
    return _responseStr;
}

void Response::dataSent(size_t bytes) {
    if (bytes >= _responseStr.length()) {
        _responseStr.clear();
    } else {
        _responseStr = _responseStr.substr(bytes);
    }
}

bool Response::isEmpty() const {
    return _responseStr.empty();
}

void Response::build() {
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << _statusCode << " " << getStatusMessage(_statusCode) << "\r\n";
    
    // Set default headers if not already set
    if (_headers.find("Content-Type") == _headers.end()) {
        _headers["Content-Type"] = "text/html; charset=UTF-8";
    }
    
    if (_headers.find("Content-Length") == _headers.end()) {
        std::ostringstream contentLength;
        contentLength << _body.length();
        _headers["Content-Length"] = contentLength.str();
    }
    
    if (_headers.find("Date") == _headers.end()) {
        char buffer[100];
        time_t now = time(0);
        struct tm* timeinfo = gmtime(&now);
        strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
        _headers["Date"] = buffer;
    }
    
    if (_headers.find("Server") == _headers.end()) {
        _headers["Server"] = "webserv/1.0";
    }
    
    // Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); 
         it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }
    
    // Empty line separating headers from body
    oss << "\r\n";
    
    // Body
    oss << _body;
    
    _responseStr = oss.str();
    _built = true;
}

std::string Response::getStatusMessage(int code) const {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}
