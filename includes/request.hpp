#pragma once

#include "config/types/consts.hpp"
#include "sessionManager.hpp"
#include "requestline.hpp"
#include "headers.hpp"
#include "cookie.hpp"
#include "fd.hpp"

#include <string>
#include <memory>

class Cookie;

enum class ReceivingBodyMode {
    NotSet,
    Chunked,
    ContentLength,
};

class Request {
private:
    void _fetch_config_from_headers();

public:
    RequestLine metadata;
    Headers headers;
    size_t contentLength;
    size_t headerPartLength;
    std::vector<Cookie> cookies;
    std::shared_ptr<SessionMetaData> session;
    ReceivingBodyMode receivingBodyMode;

    Request() = default;
    Request(std::string &buffer);
    Request(const Request &other) = default;
    Request &operator=(const Request &other) = default;
    ~Request() = default;


};
