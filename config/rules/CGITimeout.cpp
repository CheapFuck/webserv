#include "rules.hpp"

#include <iostream>
#include <cstring>

CGITimeoutRule::CGITimeoutRule() : _timeout(DEFAULT_CGI_TIMEOUT), _is_set(false) {}

static double strToSeconds(const Rule &rule) {
	bool dotFound = false;
	double num = 0;

	try { num = std::stod(rule.arguments[0].str); }
	catch (const std::invalid_argument &e) {
		throw ParserTokenException("Invalid cgitimeout value: " + rule.arguments[0].str, rule.arguments[0]);
	} catch (const std::out_of_range &e) {
		throw ParserTokenException("cgitimeout value out of range: " + rule.arguments[0].str, rule.arguments[0]);
	}

	for (size_t i = 0; i < rule.arguments[0].str.size(); ++i) {
		if (std::isdigit(rule.arguments[0].str[i])) continue;
		if (rule.arguments[0].str[i] == '.' && !dotFound) {
			dotFound = true;
			continue;
		}

		if (!rule.arguments[0].str[i])
			return num;

		if (std::strncmp(rule.arguments[0].str.c_str() + i, "ns", 3) == 0) {
			return num * 1e-9;
		} else if (std::strncmp(rule.arguments[0].str.c_str() + i, "us", 3) == 0) {
			return num * 1e-6;
		} else if (std::strncmp(rule.arguments[0].str.c_str() + i, "ms", 3) == 0) {
			return num * 1e-3;
		} else if (std::strncmp(rule.arguments[0].str.c_str() + i, "s", 2) == 0) {
			return num;
		} else if (std::strncmp(rule.arguments[0].str.c_str() + i, "m", 2) == 0) {
			return num * 60.0;
		} else if (std::strncmp(rule.arguments[0].str.c_str() + i, "h", 2) == 0) {
			return num * 3600.0;
		} else if (std::strncmp(rule.arguments[0].str.c_str() + i, "d", 2) == 0) {
			return num * 86400.0;
		} else {
			throw ParserTokenException("Invalid cgitimeout unit: " + rule.arguments[0].str.substr(i), rule.arguments[0]);
		}
	}

	throw ParserTokenException("Invalid cgitimeout format: " + rule.arguments[0].str, rule.arguments[0]);
}

CGITimeoutRule::CGITimeoutRule(const Rules &rules, bool required) : _timeout(DEFAULT_CGI_TIMEOUT), _is_set(false) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing cgitimeout rule");

	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple cgitimeout rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid cgitimeout rule format", rule);

		switch (rule.arguments[0].type) {
			case STRING:
				_timeout = strToSeconds(rule);
				_is_set = true;
				if (_timeout < 0.1)
					throw ParserTokenException("cgitimeout value too small: " + rule.arguments[0].str, rule.arguments[0]);
				break;
			case KEYWORD:
				if (rule.arguments[0].keyword == DEFAULT ||
					rule.arguments[0].keyword == AUTO)
					_is_set = true;
				else
					throw ParserTokenException("Invalid cgitimeout keyword: " + rule.arguments[0].str, rule.arguments[0]);
				break;
			default:
				throw ParserTokenException("Invalid cgitimeout argument type", rule.arguments[0]);
		}
	}
}

std::ostream& operator<<(std::ostream& os, const CGITimeoutRule& rule) {
	os << "CGITimeoutRule(timeout: " << rule.get() << ", isSet: " << (rule.isSet() ? "true" : "false") << ")";
	return os;
}
