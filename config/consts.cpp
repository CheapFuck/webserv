#include "consts.hpp"

#include <type_traits>
#include <string>

Method operator|(Method lhs, Method rhs) {
	return static_cast<Method>(
		static_cast<std::underlying_type<Method>::type>(lhs) |
		static_cast<std::underlying_type<Method>::type>(rhs)
	);
}

Method string_to_method(const std::string &str) {
	if (str == "GET") return GET;
	if (str == "POST") return POST;
	if (str == "DELETE") return DELETE;
	if (str == "PUT") return PUT;
	if (str == "HEAD") return HEAD;
	if (str == "OPTIONS") return OPTIONS;
	return UNKNOWN_METHOD;
}

std::string method_to_str(const Method &method) {
	std::string str;

	if (method & GET) str += "GET|";
	if (method & POST) str += "POST|";
	if (method & DELETE) str += "DELETE|";
	if (method & PUT) str += "PUT|";
	if (method & HEAD) str += "HEAD|";
	if (method & OPTIONS) str += "OPTIONS|";
	if (!str.empty()) str.pop_back();
	if (str.empty()) return "UNKNOWN";

	return str;
}

std::ostream& operator<<(std::ostream& os, const Method& method) {
	os << method_to_str(method);
	return os;
}