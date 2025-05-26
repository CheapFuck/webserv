#include "Request.hpp"
#include "Utils.hpp"
#include <iostream>
#include <sstream>

Request::Request() : _headersParsed(false), _contentLength(0), _chunked(false) {
}

Request::~Request() {
}

bool Request::parse(const std::string& buffer) {
    if (_headersParsed) {
        // Headers already parsed, just append body
        _body.append(buffer);
        
        // Check if we have received the full body
        if (_chunked) {
            // TODO: Implement chunked encoding parsing
            return true;
        } else {
            return _body.length() >= _contentLength;
        }
    }
    
    // Find the end of headers
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // Headers not complete yet
        return false;
    }
    
    // Parse headers
    std::string headers = buffer.substr(0, headerEnd);
    std::istringstream headerStream(headers);
    std::string line;
    
    // Parse request line
    if (!std::getline(headerStream, line)) {
        return false;
    }
    
    line = Utils::trim(line);
    if (!parseRequestLine(line)) {
        return false;
    }
    
    // Parse header lines
    while (std::getline(headerStream, line)) {
        line = Utils::trim(line);
        if (!parseHeader(line)) {
            return false;
        }
    }
    
    // Check for required headers
    if (_headers.find("Host") == _headers.end()) {
        std::cerr << "Missing required Host header" << std::endl;
        return false;
    }
    
    // Check for Content-Length or Transfer-Encoding
    if (_headers.find("Content-Length") != _headers.end()) {
        _contentLength = Utils::stringToInt(_headers["Content-Length"]);
    } else if (_headers.find("Transfer-Encoding") != _headers.end() && 
               _headers["Transfer-Encoding"] == "chunked") {
        _chunked = true;
    }
    
    _headersParsed = true;
    
    // Parse body if present
    if (headerEnd + 4 < buffer.length()) {
        _body = buffer.substr(headerEnd + 4);
        
        if (_chunked) {
            // parseChunkedBody(_body);
            return true;
        } else {
            return _body.length() >= _contentLength;
        }
    }
    
    // No body or empty body
    return _contentLength == 0 && !_chunked;
}

bool Request::parseRequestLine(const std::string& line) {
    std::vector<std::string> parts = Utils::split(line, ' ');
    if (parts.size() != 3) {
        std::cerr << "Invalid request line: " << line << std::endl;
        return false;
    }
    
    _method = parts[0];
    _path = parts[1];
    _version = parts[2];
    
    return true;
}

bool Request::parseHeader(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        std::cerr << "Invalid header line: " << line << std::endl;
        return false;
    }
    
    std::string name = Utils::trim(line.substr(0, colonPos));
    std::string value = Utils::trim(line.substr(colonPos + 1));
    
    _headers[name] = value;
    
    return true;
}

// void Request::parseChunkedBody(std::string& body) {
//     // TODO: Implement chunked encoding parsing
// }

const std::string& Request::getMethod() const {
    return _method;
}

const std::string& Request::getPath() const {
    return _path;
}

const std::string& Request::getVersion() const {
    return _version;
}

const std::string& Request::getHeader(const std::string& name) const {
    static const std::string empty;
    std::map<std::string, std::string>::const_iterator it = _headers.find(name);
    return it != _headers.end() ? it->second : empty;
}

const std::string& Request::getBody() const {
    return _body;
}
