#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <unordered_map>
#include "config/consts.hpp"

class Request {
private:
    std::string _buffer;
    Method _method;
    std::string _path;
    std::string _version;
    std::unordered_map<std::string, std::string> _headers_dict;
    std::string _body;
    bool _headers_parsed;
    size_t _content_length;

    bool _parse_request_line(const std::string& line) ;
    bool _fetch_config_from_headers();

public:
    Request();
    Request(const Request& other);
    Request& operator=(const Request& other);
    ~Request();

    void append_data(const std::string& data);
    
    bool parse_headers();
    const std::string& get_header(const std::string& key) const;
    const std::string& getBody() const;
    bool is_headers_received();

    const std::string& get_path() const;
    const std::string& get_version() const;
    const Method& get_method() const;

    bool is_complete() const;
    void reset();
};

#endif
