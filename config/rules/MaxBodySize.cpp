#include "rules.hpp"
#include <iostream>

MaxBodySizeRule::MaxBodySizeRule() : _size(), _is_set(false) {}

MaxBodySizeRule::MaxBodySizeRule(const Rules &rules, bool required) : _size(), _is_set(false) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing max body size rule");
	
	if (rules.size() > 1)
		throw ConfigParsingException("Multiple max body size rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid max body size rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid max body size argument type");

		_size = Size(rule.arguments[0].str);
		_is_set = true;
	}
}

std::ostream& operator<<(std::ostream& os, const MaxBodySizeRule& rule) {
	os << "MaxBodySizeRule(size: " << rule.get() << ")";
	return os;
}
