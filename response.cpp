#include "config/consts.hpp"
#include "config/rules/rules.hpp"
#include "methods.hpp"
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
    : _statusCode(other._statusCode), _body(other._body), _body_set(other._body_set), headers(other.headers) {}

Response& Response::operator=(const Response& other) {
    if (this != &other) {
        _statusCode = other._statusCode;
        _body = other._body;
        _body_set = other._body_set;
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
    _body_set = true;
    headers.replace(HeaderKey::ContentLength, std::to_string(body.length()));
}

/// @brief Sets default headers for the response, including Content-Type and Date.
void Response::setDefaultHeaders() {
    headers.replace(HeaderKey::Date, get_time_as_readable_string());
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

void Response::setDefaultBody(const LocationRule& rule) {
	std::map<int,Path> error_templates;
	int error_int;
    if (_body_set == true)
		return ;

	error_int = static_cast<int>(_statusCode);
	error_templates = rule.errorPages.getErrorPages();
	if (error_templates.find(error_int) == error_templates.end() 
		|| tryCreateResponseFromFile(error_templates[error_int], *this) == false)
	{
		setBody(getDefaultBodyForCode(_statusCode));
	}
}

/// @brief Sends the response to the client.
/// @param fd The file descriptor to send the response to.
/// @return Returns true if the response was sent successfully, false otherwise.
std::string Response::getAsString() const {
    std::ostringstream response_stream;
    response_stream << *this;
    DEBUG("Response as string: " << response_stream.str());
    return (response_stream.str());
}

void Response::updateFromCGIOutput(const std::string &cgiOutput) {
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        ERROR("Invalid CGI output format, no headers found.");
        setStatusCode(HttpStatusCode::InternalServerError);
        return ;
    }

    std::istringstream headerStream(cgiOutput.substr(0, headerEnd));
    headers.merge(Headers(headerStream));

    try {
        int code = std::stoi(headers.getHeader(HeaderKey::Status, "-1"));
        if (code != -1) {
            headers.remove(HeaderKey::Status);
            setStatusCode(static_cast<HttpStatusCode>(code));
            setBody(cgiOutput.substr(headerEnd + 4));
        } else {
            ERROR("Invalid status code in CGI output headers: " << headers.getHeader(HeaderKey::Status));
            setStatusCode(HttpStatusCode::InternalServerError);
        }
    } catch (const std::exception &e) {
        ERROR("Failed to parse status code from CGI output headers: " << e.what());
        setStatusCode(HttpStatusCode::InternalServerError);
    }
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
