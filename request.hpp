#pragma once

#include "config/consts.hpp"
#include "requestline.hpp"
#include "headers.hpp"

#include <string>

#define READ_BUFFER_SIZE 4096

class Request {
private:
    std::string _buffer;
    size_t _contentLength = 0;

    bool _headersParsed = false;

    bool _fetch_config_from_headers();
    bool _parse_request_headers();
    
public:
    RequestLine metadata;
    Headers headers;

    Request() = default;
    Request(const Request &other);
    Request &operator=(const Request &other);
    ~Request() = default;

    bool read(const int fd);
    bool isComplete() const;

    inline const std::string &getBody() const { return _buffer; }
    inline size_t getContentLength() const { return _contentLength; }
};
