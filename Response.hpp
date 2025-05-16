#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
private:
    int _statusCode;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _responseStr;
    bool _built;
    
public:
    Response();
    ~Response();
    
    void setStatusCode(int code);
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
