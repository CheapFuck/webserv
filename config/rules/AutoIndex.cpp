#include "rules.hpp"

#include <iostream>

AutoIndexRule::AutoIndexRule() : _autoindex(false), _is_set(false) {}

AutoIndexRule::AutoIndexRule(const Rules &rules, bool required) : _autoindex(true), _is_set(false) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing autoindex rule");
	
	if (rules.size() > 1)
		throw ConfigParsingException("Multiple autoindex rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid autoindex rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid autoindex argument type");

		std::string arg = rule.arguments[0].str;
		if (arg == "on") {
			_autoindex = true;
			_is_set = true;
		} else if (arg == "off") {
			_autoindex = false;
			_is_set = true;
		} else {
			throw ConfigParsingException("Invalid autoindex value: " + arg);
		}
	}
}

inline bool AutoIndexRule::get() const {
	return _autoindex;
}

inline bool AutoIndexRule::isSet() const {
	return _is_set;
}

std::ostream& operator<<(std::ostream& os, const AutoIndexRule& rule) {
	os << "AutoIndexRule(autoindex: " << (rule.get() ? "on" : "off") << ", isSet: " << rule.isSet() << ")";
	return os;
}
