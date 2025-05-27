#pragma once

#include "config/consts.hpp"

#include <sstream>
#include <string>
#include <map>



class Headers {
private:
    std::map<std::string, std::string> _headers;

public:
    Headers();
    Headers(std::istringstream &source);
    Headers(const Headers &other);
    Headers &operator=(const Headers &other);
    ~Headers();

    bool isValid() const;

    void add(const std::string &key, const std::string &value);
    void add(HeaderKey key, const std::string &value);
    void replace(HeaderKey key, const std::string &value);

    const std::string &getHeader(HeaderKey key) const;
    const std::string &getHeader(HeaderKey key, const std::string &default_value) const;

    const std::map<std::string, std::string>& getHeaders() const;
};

std::ostream &operator<<(std::ostream &os, const Headers &headers);
std::ostringstream &operator<<(std::ostringstream &os, const Headers &headers);