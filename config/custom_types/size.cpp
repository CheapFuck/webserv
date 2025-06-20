#include "../rules/rules.hpp"
#include "../config.hpp"

#include <iostream>
#include <string.h>
#include <cstring>

Size::Size(const Rule &rule): _size(0) {
	try { _size = std::stoul(rule.arguments[0].str); }
	catch (const std::invalid_argument &e) {
		throw ParserTokenException("Invalid size value: " + rule.arguments[0].str, rule.arguments[0]);
	}
	catch (const std::out_of_range &e) {
		throw ParserTokenException("Size value out of range: " + rule.arguments[0].str, rule.arguments[0]);
	}

	if (_size == 0)
		throw ParserTokenException("Invalid size value: " + rule.arguments[0].str, rule.arguments[0]);

	for (size_t i = 0; i < rule.arguments[0].str.size(); i++) {
		if (!isdigit(rule.arguments[0].str[i])) {
			if (strncmp(rule.arguments[0].str.c_str() + i, "kb", 3) == 0 || strncmp(rule.arguments[0].str.c_str() + i, "Kb", 3) == 0)
				_size *= 1024;
			else if (strncmp(rule.arguments[0].str.c_str() + i, "mb", 3) == 0 || strncmp(rule.arguments[0].str.c_str() + i, "Mb", 3) == 0)
				_size *= 1024 * 1024;
			else if (strncmp(rule.arguments[0].str.c_str() + i, "gb", 3) == 0 || strncmp(rule.arguments[0].str.c_str() + i, "Gb", 3) == 0)
				_size *= 1024 * 1024 * 1024;
			else
				throw ParserTokenException("Invalid size multiplier: " + rule.arguments[0].str.substr(i), rule.arguments[0]);
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

/// @brief Get the size value
/// @return The size value in bytes
size_t Size::get() const {
	return _size;
}

std::ostream &operator<<(std::ostream &os, const Size &size) {
	os << "Size(size: " << size.get() << ")";
	return os;
}
