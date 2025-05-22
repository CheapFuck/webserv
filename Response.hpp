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
    
	const std::unordered_map<std::string, std::string> getHeaders() const;
    std::string toString() const;
	static constexpr const char* _protocol = "HTTP";
	static constexpr const char* _tls_version = "1.1";
	size_t getStatusCode() const;
    const char* getStatusMessage() const;
};

//     std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";




std::ostream &operator<<(std::ostream &os, const Response& obj);

#endif // RESPONSE_HPP
