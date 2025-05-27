#include "rules.hpp"

#include <iostream>

ErrorPageRule::ErrorPageRule(const Object &obj, bool required) :
	_default_page(Path::createDummy()) {

	for (const Rule &rule : obj) {
		if (rule.key != ERROR_PAGE) continue;

		int rule_size = rule.arguments.size();

		if (rule_size < 1)
			throw ConfigParsingException("Invalid error page rule");
		if (rule.arguments[rule_size - 1].type != STRING)
			throw ConfigParsingException("Invalid error page argument type");
		
		Path path(rule.arguments[rule_size - 1].str);
		if (rule_size == 1) {
			_default_page = path;
			continue;
		}

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ConfigParsingException("Invalid error page argument type");

			int code = std::stoi(arg.str);
			if (code < 100 || code > 599)
				throw ConfigParsingException("Error page code out of range: " + arg.str);

			_error_pages[code] = Path(path);
			if (--rule_size == 1) break;
		}
	}

	if (!_default_page.isValid() && required)
		throw ConfigParsingException("Invalid default error page path");
}

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
