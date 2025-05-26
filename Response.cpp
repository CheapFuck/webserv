#include "config/consts.hpp"
#include "Response.hpp"
#include <sstream>
#include <iostream> 
#include <ctime>
#include <unordered_map>

Response::Response() : _statusCode(HttpStatusCode::OK), _body("") {
    setHeader("Content-Length", "0");
    setHeader("Content-Type", "text/html");
    setHeader("Date", std::to_string(std::time(0)));
}

Response::~Response() {
}

Response::Response(const Response& other) 
{
    _statusCode = other._statusCode;
    _body = other._body;
    _headers_dict = other._headers_dict;
}

Response& Response::operator=(const Response& other) 
{
    if (this != &other) {
        _statusCode = other._statusCode;
        _body = other._body;
        _headers_dict = other._headers_dict;
    }
    return *this;
}

void Response::setStatusCode(HttpStatusCode code) {
    _statusCode = code;
}

void Response::setHeader(const std::string& name, const std::string& value) 
{
    _headers_dict[name] = value;
}

void Response::setBody(const std::string& body) {
    setHeader("Content-Length", std::to_string(body.length()));
    _body = body;
}

HttpStatusCode Response::getStatusCode() const
{
	return (_statusCode);
}

const std::unordered_map<std::string, std::string>& Response::getHeaders() const
{
	return (_headers_dict);
}

const std::string &Response::getBody() const
{
    return (_body);
}

std::string Response::toString() const
{
    std::ostringstream os;

    os << *this;
    return (os.str());
}

std::ostream &operator<<(std::ostream &os, const Response& obj)
{
	const std::unordered_map<std::string, std::string>& _readonly = obj.getHeaders();

    os << obj._protocol << "/" <<  obj._tls_version << " " << static_cast<int>(obj.getStatusCode()) \
	<< " " << obj.getStatusCode() << "\r\n";
	for (std::unordered_map<std::string, std::string>::const_iterator iter = _readonly.begin(); iter != _readonly.end(); iter++)
	{		
		os << iter->first << ": " << iter->second << "\r\n";
	}	
	os << "\r\n";
    if (obj.getBody().length() > 0)
        os << obj.getBody() << "\r\n";
    return (os);
}

std::ostringstream &operator<<(std::ostringstream &os, const Response& obj)
{
	const std::unordered_map<std::string, std::string>& _readonly = obj.getHeaders();

    os << obj._protocol << "/" <<  obj._tls_version << " " << static_cast<int>(obj.getStatusCode()) \
	<< " " << obj.getStatusCode() << "\r\n";
	for (std::unordered_map<std::string, std::string>::const_iterator iter = _readonly.begin(); iter != _readonly.end(); iter++)
	{		
		os << iter->first << ": " << iter->second << "\r\n";
	}	
	os << "\r\n";
    if (obj.getBody().length() > 0)
        os << obj.getBody() << "\r\n";
    return (os);
}
