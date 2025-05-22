#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <unordered_map>

class Response {
private:
    size_t _statusCode;
    std::string _buffer; // Make this a custom buffer class for the request
    std::unordered_map<std::string, std::string> _headers_dict;
	std::string _body;

    
public:
    Response();
    ~Response();
    
    void setStatusCode(size_t code);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    
	const std::unordered_map<std::string, std::string>& getHeaders() const;
    std::string toString() const;
	static constexpr const char* _protocol = "HTTP";
	static constexpr const char* _tls_version = "1.1";
	size_t getStatusCode() const;
    const char* getStatusMessage() const;
    const std::string &getBody() const;
};

std::ostream &operator<<(std::ostream &os, const Response& obj);
std::ostringstream &operator<<(std::ostringstream &os, const Response& obj);

#endif // RESPONSE_HPP
