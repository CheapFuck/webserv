#include "requestline.hpp"
#include "Utils.hpp"

#include <sstream>

RequestLine::RequestLine() : _method(UNKNOWN_METHOD), _path(""), _version("HTTP/1.1") {}

RequestLine::RequestLine(std::istringstream &source) {
    std::string method_str;
    source >> method_str >> _path >> _version;

    _path = Utils::trim(_path);
    _version = Utils::trim(_version);
    _method = stringToMethod(Utils::trim(method_str));
}

RequestLine::RequestLine(const RequestLine &other)
    : _method(other._method), _path(other._path), _version(other._version) {}

RequestLine &RequestLine::operator=(const RequestLine &other) {
    if (this != &other) {
        _method = other._method;
        _path = other._path;
        _version = other._version;
    }
    return *this;
}

RequestLine::~RequestLine() {}

/// Checks if the request line is valid.
bool RequestLine::isValid() const {
    return (_method != UNKNOWN_METHOD && !_path.empty() && !_version.empty());
}

/// Returns the HTTP method of the request line.
inline const Method &RequestLine::getMethod() const {
    return _method;
}

/// Returns the path (url) of the request line.
inline const std::string &RequestLine::getPath() const {
    return _path;
}

/// Returns the HTTP version of the request line.
inline const std::string &RequestLine::getVersion() const {
    return _version;
}

std::ostream &operator<<(std::ostream &os, const RequestLine &request_line) {
    os << request_line.getMethod() << " " << request_line.getPath() << " " << request_line.getVersion();
    return os;
}
