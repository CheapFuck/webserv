#pragma once

#include "config/types/consts.hpp"
#include "sessionManager.hpp"
#include "requestline.hpp"
#include "headers.hpp"
#include "cookie.hpp"

#include <string>

#define READ_BUFFER_SIZE 4096

class Cookie;

class Request {
private:
    size_t _contentLength = 0;
    std::vector<Cookie> _cookies;
    std::string _body;

    bool _headersParsed = false;

    bool _fetch_config_from_headers();

public:
    RequestLine metadata;
    Headers headers;
    std::shared_ptr<SessionMetaData> session;

    Request() = default;
    Request(const Request &other);
    Request &operator=(const Request &other);
    ~Request() = default;

    bool isComplete(const std::string &buffer) const;
    bool parseRequestHeaders(std::string &buffer);

    inline const std::string &getBody() const { return _body; }
    inline void setBody(const std::string &body) { _body = body; }
    inline const std::vector<Cookie> &getCookies() const { return _cookies; }
    inline size_t getContentLength() const { return _contentLength; }
};
