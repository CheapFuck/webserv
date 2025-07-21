#include "config/parserExceptions.hpp"
#include "config/rules/rules.hpp"
#include "config/types/customTypes.hpp"
#include "config/config.hpp"

#include <iostream>
#include <string.h>
#include <cstring>

Size::Size(size_t size) : _size(size) {}

Size::Size(const std::string &str) {
    _size = std::stoul(str);

	if (_size == 0)
        throw std::invalid_argument("Size cannot be zero");
    
    for (size_t i = 0; i < str.size(); i++) {
        if (!isdigit(str[i])) {
            if (strncmp(str.c_str() + i, "kb", 3) == 0 || strncmp(str.c_str() + i, "Kb", 3) == 0)
                _size *= 1024;
            else if (strncmp(str.c_str() + i, "mb", 3) == 0 || strncmp(str.c_str() + i, "Mb", 3) == 0)
                _size *= 1024 * 1024;
            else if (strncmp(str.c_str() + i, "gb", 3) == 0 || strncmp(str.c_str() + i, "Gb", 3) == 0)
                _size *= 1024 * 1024 * 1024;
            else
                throw std::invalid_argument("Invalid size multiplier");
            break;
        }
    }
};

/// @brief Get the size value
/// @return The size value in bytes
size_t Size::get() const {
	return _size;
}

std::ostream &operator<<(std::ostream &os, const Size &size) {
	os << "Size(size: " << size.get() << ")";
	return os;
}
