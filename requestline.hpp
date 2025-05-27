#pragma once

#include "config/consts.hpp"

#include <sstream>

class RequestLine {
private:
    Method _method;
    std::string _path;
    std::string _version;

public:
    RequestLine();
    RequestLine(std::istringstream &source);
    RequestLine(const RequestLine &other);
    RequestLine &operator=(const RequestLine &other);
    ~RequestLine();

    bool isValid() const;

    const Method &getMethod() const;
    const std::string &getPath() const;
    const std::string &getVersion() const;
};

std::ostream &operator<<(std::ostream &os, const RequestLine &request_line);