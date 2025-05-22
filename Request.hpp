#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <unordered_map>
#include "config/consts.hpp"

class Request {
private:
    std::unordered_map<std::string, std::string> _headers_dict;

public:
    Request();
    ~Request();
    
    bool parse_headers();
    const std::string& get_header(const std::string& key) const;
    const std::string& getBody() const;
    bool is_headers_received();
    std::string _buffer;

private:

};

#endif // REQUEST_HPP
