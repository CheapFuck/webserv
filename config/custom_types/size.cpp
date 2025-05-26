#include "../config.hpp"
#include "../rules/rules.hpp"

#include <iostream>
#include <string.h>
#include <cstring>

Size::Size(const std::string &str) {
	_size = std::stoul(str);

	if (_size == 0)
		throw ConfigParsingException("Invalid size value: " + str);

	for (size_t i = 0; i < str.size(); i++) {
		if (!isdigit(str[i])) {
			if (strncmp(str.c_str() + i, "kb", 3) == 0 || strncmp(str.c_str() + i, "Kb", 3) == 0)
				_size *= 1024;
			else if (strncmp(str.c_str() + i, "mb", 3) == 0 || strncmp(str.c_str() + i, "Mb", 3) == 0)
				_size *= 1024 * 1024;
			else if (strncmp(str.c_str() + i, "gb", 3) == 0 || strncmp(str.c_str() + i, "Gb", 3) == 0)
				_size *= 1024 * 1024 * 1024;
			else
				throw ConfigParsingException("Invalid size multiplier: " + str);
			break;
		}
	}
};

Size::Size() : _size(0) {}

Size::Size(const Size &other) : _size(other._size) {}

Size &Size::operator=(const Size &other) {
	if (this != &other) {
		_size = other._size;
	}
	return *this;
}

size_t Size::get() const {
	return _size;
}

std::ostream &operator<<(std::ostream &os, const Size &size) {
	os << "Size(size: " << size.get() << ")";
	return os;
}
