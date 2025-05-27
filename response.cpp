#include "config/consts.hpp"
#include "response.hpp"
#include "headers.hpp"
#include "print.hpp"

#include <sys/socket.h>
#include <sstream>
#include <ctime>

/// @brief Returns the current time as a formatted string in GMT.
static std::string get_time_as_readable_string() {
    std::time_t now = std::time(nullptr);
    char buffer[150];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
    return std::string(buffer);
}

Response::Response() : _statusCode(HttpStatusCode::OK), _body(""), headers() {}

Response::Response(const Response& other) 
    : _statusCode(other._statusCode), _body(other._body), headers(other.headers) {}

Response& Response::operator=(const Response& other) {
    if (this != &other) {
        _statusCode = other._statusCode;
        _body = other._body;
        headers = other.headers;
    }
    return *this;
}

Response::~Response() {}

/// @brief Sets the status code for the response.
/// @param code The HTTP status code to set for the response.
void Response::setStatusCode(HttpStatusCode code) {
    _statusCode = code;
}

/// @brief Sets the body of the response and updates the Content-Length header accordingly.
/// @param body The body content to set for the response.
void Response::setBody(const std::string& body) {
    _body = body;
    headers.replace(HeaderKey::ContentLength, std::to_string(body.length()));
}

/// @brief Sets default headers for the response, including Content-Type and Date.
void Response::setDefaultHeaders() {
    headers.replace(HeaderKey::Date, get_time_as_readable_string());
    headers.replace(HeaderKey::ContentType, "text/html");
    headers.replace(HeaderKey::ContentLength, std::to_string(_body.length()));
}

/// @brief Returns the status code of the response.
HttpStatusCode Response::getStatusCode() const {
    return _statusCode;
}

/// @brief Returns the body of the response.
const std::string& Response::getBody() const {
    return _body;
}

/// @brief Sends the response to the client.
/// @param fd The file descriptor to send the response to.
/// @return Returns true if the response was sent successfully, false otherwise.
bool Response::sendToClient(int fd) const {
    std::ostringstream response_stream;
    response_stream << *this;

    std::string response_str = response_stream.str();
    ssize_t bytes_sent = send(fd, response_str.c_str(), response_str.length(), 0);
    if (bytes_sent < 0) {
        ERROR("Failed to send response to client");
        return (false);
    }
    DEBUG("Sent " << bytes_sent << " bytes to client | " << static_cast<int>(_statusCode) << " (" << _statusCode << ")");
    return (true);
}

std::ostream& operator<<(std::ostream& os, const Response& obj) {
    os << Response::protocol << "/" << Response::tlsVersion << " "
       << static_cast<int>(obj.getStatusCode()) << " "
       << getStatusCodeAsStr(obj.getStatusCode()) << "\r\n";
    os << obj.headers;
    os << "\r\n" << obj.getBody();
    return os;
}

std::ostringstream& operator<<(std::ostringstream& os, const Response& obj) {
    os << Response::protocol << "/" << Response::tlsVersion << " "
       << static_cast<int>(obj.getStatusCode()) << " "
       << getStatusCodeAsStr(obj.getStatusCode()) << "\r\n";
    os << obj.headers;
    os << "\r\n" << obj.getBody();
    return os;
}
