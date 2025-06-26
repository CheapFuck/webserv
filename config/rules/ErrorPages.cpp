#include "rules.hpp"

#include <iostream>

ErrorPageRule::ErrorPageRule(): _error_pages() {}

ErrorPageRule::ErrorPageRule(const Rules &rules, bool required): _error_pages() {
	if (rules.empty() && required)
		throw ParserMissingException("Missing error page rule");

	for (const Rule &rule : rules) {
		size_t rule_size = rule.arguments.size();

		if (rule_size < 2)
			throw ParserTokenException("Invalid error page rule format", rule);
		if (rule.arguments[rule_size - 1].type != STRING)
			throw ParserTokenException("Invalid error page path argument type", rule.arguments[rule_size - 1]);

		Path path(rule.arguments[rule_size - 1].str);
		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ParserTokenException("Invalid error page code argument type", arg);

			int code;
			try {code = std::stoi(arg.str); }
			catch (const std::invalid_argument &e) {
				throw ParserTokenException("Invalid error page code: " + arg.str, arg);
			}
			catch (const std::out_of_range &e) {
				throw ParserTokenException("Error page code out of range: " + arg.str, arg);
			}
			if (code < 100 || code > 599)
				throw ParserTokenException("Error page code must be between 100 and 599: " + arg.str, arg);

			_error_pages[code] = path;
			if (--rule_size == 1) break;
		}
	}
}

/// @brief Update this error page rule with the default rule
/// @param defaultRule The default error page rule to merge with this one.
void ErrorPageRule::updateFromDefault(const ErrorPageRule &defaultRule) {
	for (const auto &pair : defaultRule.getErrorPages()) {
		int code = pair.first;
		
		if (_error_pages.find(code) == _error_pages.end())
			_error_pages[code] = pair.second;
	}
}

/// @brief Returns the error page for the given code.
/// @param code The HTTP status code for which to retrieve the error page.
/// @return The path to the error page as a string, or an empty string if not found.
/// If the code is not found, it will return the wildcard error page if it exists.
std::string ErrorPageRule::getErrorPage(int code) const {
	auto it = _error_pages.find(code);
	if (it != _error_pages.end())
		return it->second.str();

	auto wildcardIt = _error_pages.find(-1);
	if (wildcardIt != _error_pages.end())
		return wildcardIt->second.str();

	return "";
}

inline const std::map<int, Path> &ErrorPageRule::getErrorPages() const {
	return _error_pages;
}

std::ostream& operator<<(std::ostream& os, const ErrorPageRule& rule) {
	os << "ErrorPageRule(custom_error_pages: {";
	for (const auto &pair : rule.getErrorPages()) {
		os << pair.first << ": " << pair.second << ", ";
	}
	os << "})";
	return os;
}
