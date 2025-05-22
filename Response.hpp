#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
private:
    size_t _statusCode;
    std::string _buffer; // Make this a custom buffer class for the request
    std::unordered_map<std::string, std::string> _headers_dict;
    
public:
    Response();
    ~Response();
    
    void setStatusCode(size_t code);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    
    std::string toString();
    void dataSent(size_t bytes);
    bool isEmpty() const;
    
private:
    void build();
    std::string getStatusMessage(int code) const;
};

#endif // RESPONSE_HPP
