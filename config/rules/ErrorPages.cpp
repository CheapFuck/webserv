#include "rules.hpp"

#include <iostream>

ErrorPageRule::ErrorPageRule(const Rules &rules, bool required) :
	_default_page(Path::createDummy()) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing error page rule");

	for (const Rule &rule : rules) {
		int rule_size = rule.arguments.size();

		if (rule_size < 2)
			throw ConfigParsingException("Invalid error page rule");
		if (rule.arguments[rule_size - 1].type != STRING)
			throw ConfigParsingException("Invalid error page argument type");

		Path path(rule.arguments[rule_size - 1].str);
		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ConfigParsingException("Invalid error page argument type");

			int code = std::stoi(arg.str);
			if (code < 100 || code > 599)
				throw ConfigParsingException("Error page code out of range: " + arg.str);

			_error_pages[code] = path;
			if (--rule_size == 1) break;
		}
	}

	if (!_default_page.isValid() && required)
		throw ConfigParsingException("Invalid default error page path");
}

/// @brief Update this error page rule with the default rule
/// @param defaultRule The default error page rule to merge with this one.
void ErrorPageRule::updateFromDefault(const ErrorPageRule &defaultRule) {
	for (const auto &pair : defaultRule.getErrorPages()) {
		int code = pair.first;
		
		if (_error_pages.find(code) == _error_pages.end())
			_error_pages[code] = pair.second;
	}

	if (!_default_page.isValid() && defaultRule._default_page.isValid())
		_default_page = defaultRule._default_page;
}

/// @brief Get an error page
/// @param code The HTTP status code for which to retrieve the error page.
/// @return A reference to the Path object representing the error page for the given code.
/// If no specific page is set for the code, it returns the default error page (which may be empty).
const Path &ErrorPageRule::getErrorPage(int code) const {
	auto it = _error_pages.find(code);
	if (it == _error_pages.end())
		return _default_page;
	return it->second;
}

inline const std::map<int, Path> &ErrorPageRule::getErrorPages() const {
	return _error_pages;
}

std::ostream& operator<<(std::ostream& os, const ErrorPageRule& rule) {
	os << "ErrorPageRule(custom_error_pages: {";
	for (const auto &pair : rule.getErrorPages()) {
		os << pair.first << ": " << pair.second << ", ";
	}
	os << "}, default_error_page: " << rule.getErrorPage(0) << ")";
	return os;
}
