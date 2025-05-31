#include "headers.hpp"
#include "print.hpp"
#include "Utils.hpp"

#include <map>
#include <vector>
#include <string>
#include <sstream>

Headers::Headers() {}

static std::string removePortFromHostValue(const std::string& value)
{
    std::size_t pos = value.find(":");

    if (pos == std::string::npos)
        return (value);
    return (value.substr(0, pos));
}

Headers::Headers(std::istringstream &source) {
    std::string line;
    bool first_line = true;
    constexpr std::string_view host_string = "Host";

    while (std::getline(source, line, '\n')) {
        line = Utils::trim(line);
        if (line.empty()) {
            if (!first_line)
                break;
            first_line = false;
            continue;
        }

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            ERROR("Invalid header line: " << line);
            continue;
        }

        std::string key = Utils::trim(line.substr(0, colon));
        std::string value = Utils::trim(line.substr(colon + 1));
        if (key == host_string)
            value = removePortFromHostValue(value);
        add(key, value);
    }
}

Headers::Headers(const Headers &other) : _headers(other._headers) {}

Headers &Headers::operator=(const Headers &other) {
    if (this != &other) {
        _headers = other._headers;
    }
    return *this;
}

Headers::~Headers() {}

/// @brief Checks if the headers map is empty.
inline bool Headers::isValid() const {
    return !_headers.empty();
}

/// @brief Adds a new header.
void Headers::add(const std::string &key, const std::string &value) {
    _headers.insert(std::make_pair(key, value));
}

/// @brief Adds a new header.
void Headers::add(HeaderKey key, const std::string &value) {
    _headers.insert(std::make_pair(headerKeyToString(key), value));
}

/// @brief Replaces the value of an existing header - even if multiple headers have been set already.
void Headers::replace(HeaderKey key, const std::string &value) {
    _headers.erase(headerKeyToString(key));
    _headers.insert(std::make_pair(headerKeyToString(key), value));
}

/// @brief Retrieves the value of a header by its key. Throws if the key does not exist.
const std::string &Headers::getHeader(HeaderKey key) const {
    auto it = _headers.find(headerKeyToString(key));
    if (it != _headers.end())
        return it->second;
    throw std::out_of_range("Header not found: " + headerKeyToString(key));
}

/// @brief Retrieves the value of a header by its key, returning a default value
/// if the key does not exist.
const std::string &Headers::getHeader(HeaderKey key, const std::string &default_value) const {
    auto it = _headers.find(headerKeyToString(key));
    if (it != _headers.end())
        return it->second;
    return default_value;
}

/// @brief Returns a constant reference to the internal headers map.
const std::multimap<std::string, std::string> &Headers::getHeaders() const {
    return (_headers);
}

std::ostream &operator<<(std::ostream &os, const Headers &headers) {
    for (const auto &header : headers.getHeaders()) {
        os << header.first << ": " << header.second << "\r\n";
    }
    return os;
}

std::ostringstream &operator<<(std::ostringstream &os, const Headers &headers) {
    for (const auto &header : headers.getHeaders()) {
        os << header.first << ": " << header.second << "\r\n";
    }
    return os;
}
