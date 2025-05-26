#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class Request {
private:
    std::string _method;
    std::string _path;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _headersParsed;
    size_t _contentLength;
    bool _chunked;
    
public:
    Request();
    ~Request();
    
    bool parse(const std::string& buffer);
    
    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getVersion() const;
    const std::string& getHeader(const std::string& name) const;
    const std::string& getBody() const;
    
private:
    bool parseRequestLine(const std::string& line);
    bool parseHeader(const std::string& line);
    // void parseChunkedBody(std::string& body);
};

#endif // REQUEST_HPP
