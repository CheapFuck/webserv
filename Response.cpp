#include "Response.hpp"
#include <sstream>
#include <ctime>
#include <unordered_map>

Response::Response() : _statusCode(0), _body("") {
}

Response::~Response() {
}

void Response::setStatusCode(size_t code) {
    _statusCode = code;
}

void Response::setHeader(const std::string& name, const std::string& value) 
{
    _headers_dict[name] = value;
}

void Response::setBody(const std::string& body) {
    _body = body;
}

size_t Response::getStatusCode() const
{
	return (_statusCode);
}

const std::unordered_map<std::string, std::string> Response::getHeaders() const
{
	return (_headers_dict);
}

std::string Response::toString() const
{
    std::ostringstream os;

    os << this;
    std::string as_string = os.str();
    return (as_string);
}

std::ostream &operator<<(std::ostream &os, const Response& obj)
{
	std::unordered_map<std::string, std::string> _readonly;

	_readonly = obj.getHeaders();
    os << obj._protocol << "/" <<  obj._tls_version << " " << obj.getStatusCode() \
	<< " " << obj.getStatusMessage() << "\r\n";
	for (std::unordered_map<std::string, std::string>::const_iterator iter = _readonly.begin(); iter != _readonly.end(); iter++)
	{		
		os << iter->first << ":" << iter->second << "\r\n";
	}	
	os << "\r\n";
    return (os);
}


const char* Response::getStatusMessage() const {
    switch (_statusCode) {
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


