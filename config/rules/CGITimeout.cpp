#include "rules.hpp"

#include <iostream>
#include <cstring>

CGITimeoutRule::CGITimeoutRule() : _timeout(DEFAULT_CGI_TIMEOUT), _is_set(false) {}

static double strToSeconds(const std::string &str) {
	bool dotFound = false;
	double num = std::stod(str);

	for (size_t i = 0; i < str.size(); ++i) {
		if (std::isdigit(str[i])) continue;
		if (str[i] == '.' && !dotFound) {
			dotFound = true;
			continue;
		}

		if (!str[i])
			return num;

		if (std::strncmp(str.c_str() + i, "ns", 3) == 0) {
			return num * 1e-9;
		} else if (std::strncmp(str.c_str() + i, "us", 3) == 0) {
			return num * 1e-6;
		} else if (std::strncmp(str.c_str() + i, "ms", 3) == 0) {
			return num * 1e-3;
		} else if (std::strncmp(str.c_str() + i, "s", 2) == 0) {
			return num;
		} else if (std::strncmp(str.c_str() + i, "m", 2) == 0) {
			return num * 60.0;
		} else if (std::strncmp(str.c_str() + i, "h", 2) == 0) {
			return num * 3600.0;
		} else if (std::strncmp(str.c_str() + i, "d", 2) == 0) {
			return num * 86400.0;
		} else {
			throw ConfigParsingException("Invalid cgitimeout unit: " + str);
		}
	}

	throw ConfigParsingException("Invalid cgitimeout format: " + str);
}

CGITimeoutRule::CGITimeoutRule(const Rules &rules, bool required) : _timeout(DEFAULT_CGI_TIMEOUT), _is_set(false) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing cgitimeout rule");

	if (rules.size() > 1)
		throw ConfigParsingException("Multiple cgitimeout rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid cgitimeout rule");

		switch (rule.arguments[0].type) {
			case STRING:
				_timeout = strToSeconds(rule.arguments[0].str);
				_is_set = true;
				break;
			case KEYWORD:
				if (rule.arguments[0].keyword == DEFAULT ||
					rule.arguments[0].keyword == AUTO)
					_is_set = true;
				throw ConfigParsingException("Invalid cgitimeout keyword: " + rule.arguments[0].str);
			default:
				throw ConfigParsingException("Invalid cgitimeout argument type: " + std::to_string(rule.arguments[0].type));
		}
	}
}

std::ostream& operator<<(std::ostream& os, const CGITimeoutRule& rule) {
	os << "CGITimeoutRule(timeout: " << rule.get() << ", isSet: " << (rule.isSet() ? "true" : "false") << ")";
	return os;
}
