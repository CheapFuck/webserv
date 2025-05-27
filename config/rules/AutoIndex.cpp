#include "rules.hpp"

#include <iostream>

AutoIndexRule::AutoIndexRule(const Object &obj, bool required) : _autoindex(true), _is_set(false) {
	int rule_count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != AUTOINDEX) continue;
		if (++rule_count > 1)
			throw ConfigParsingException("Duplicate autoindex rule");

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

	if (!_is_set && required)
		throw ConfigParsingException("Missing autoindex rule");
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
