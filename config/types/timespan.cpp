#include "../parserExceptions.hpp"
#include "../rules/rules.hpp"
#include "customTypes.hpp"
#include "../config.hpp"

#include <iostream>
#include <string.h>
#include <cstring>

Timespan::Timespan(std::string str) {
    size_t pos;
    _seconds = std::stod(str, &pos);

    if (_seconds < 0)
        throw std::invalid_argument("Timespan cannot be negative");

    if (pos == str.size())
        return;
    if (std::strncmp(str.c_str() + pos, "ns", 3) == 0)
        _seconds *= 1e-9;
    else if (std::strncmp(str.c_str() + pos, "us", 3) == 0)
        _seconds *= 1e-6;
    else if (std::strncmp(str.c_str() + pos, "ms", 3) == 0)
        _seconds *= 1e-3;
    else if (std::strncmp(str.c_str() + pos, "s", 2) == 0)
        _seconds *= 1;
    else if (std::strncmp(str.c_str() + pos, "m", 2) == 0)
        _seconds *= 60;
    else if (std::strncmp(str.c_str() + pos, "h", 2) == 0)
        _seconds *= 3600;
    else if (std::strncmp(str.c_str() + pos, "d", 2) == 0)
        _seconds *= 86400;
    else if (std::strncmp(str.c_str() + pos, "w", 2) == 0)
        _seconds *= 604800;
    else if (std::strncmp(str.c_str() + pos, "y", 2) == 0)
        _seconds *= 31536000;
    else
        throw std::invalid_argument("Invalid timespan multiplier");
}

Timespan::Timespan(double seconds) : _seconds(seconds) {
    if (_seconds < 0)
        throw std::invalid_argument("Timespan cannot be negative");
}

/// @brief Get the timespan in seconds.
double Timespan::getSeconds() const {
    return _seconds;
}
