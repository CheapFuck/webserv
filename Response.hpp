#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "config/consts.hpp"

#include <string>
#include <unordered_map>

class Response {
private:
    HttpStatusCode _statusCode;
    std::unordered_map<std::string, std::string> _headers_dict;
	std::string _body;

public:
    Response();
    Response(const Response& other);
    Response& operator=(const Response& other);
    ~Response();
    
    void setStatusCode(HttpStatusCode code);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    
	const std::unordered_map<std::string, std::string>& getHeaders() const;
    std::string toString() const;
	static constexpr const char* _protocol = "HTTP";
	static constexpr const char* _tls_version = "1.1";
	HttpStatusCode getStatusCode() const;
    const char* getStatusMessage() const;
    const std::string &getBody() const;
};

std::ostream &operator<<(std::ostream &os, const Response& obj);
std::ostringstream &operator<<(std::ostringstream &os, const Response& obj);

#endif // RESPONSE_HPP
