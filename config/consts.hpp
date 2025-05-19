#pragma once

#include <string>

enum Method {
	UNKNOWN_METHOD = 0,
	GET = 1 << 0,
	POST = 1 << 1,
	DELETE = 1 << 2,
	PUT = 1 << 3,
	HEAD = 1 << 4,
	OPTIONS = 1 << 5,
};

#define ALL_METHODS (GET | POST | DELETE | PUT | HEAD | OPTIONS)

Method operator|(Method lhs, Method rhs);
Method string_to_method(const std::string &str);
std::string method_to_str(const Method &method);