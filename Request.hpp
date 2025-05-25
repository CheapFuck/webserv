#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <unordered_map>

#include "config/consts.hpp"
#include "config/rules/rules.hpp"

class Request {
private:
    std::string _buffer;
    Method _method;
    std::string _path;
    std::string _version;
    size_t _max_body_size;
    std::unordered_map<std::string, std::string> _headers_dict;
    bool _headers_parsed;
    size_t _content_length;

    bool _parse_request_line(const std::string& line) ;
    bool _fetch_config_from_headers();

public:
    Request(size_t max_body_size);
    Request(const Request& other);
    Request& operator=(const Request& other);
    ~Request();

    void append_data(const std::string& data);
    
    bool parse_headers();
    const std::string& get_header(const std::string& key) const;
    const std::string& get_header(const std::string& key, const std::string& default_value) const;
    const std::string& getBody() const;
    bool is_headers_received();
    bool is_body_within_limits() const;

    const std::string& get_path() const;
    const std::string& get_version() const;
    const Method& get_method() const;

    bool is_complete() const;
    void reset();
};

#endif
