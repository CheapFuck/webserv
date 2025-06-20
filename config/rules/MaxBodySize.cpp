#include "rules.hpp"
#include <iostream>

MaxBodySizeRule::MaxBodySizeRule() : _size(), _is_set(false) {}

MaxBodySizeRule::MaxBodySizeRule(const Rules &rules, bool required) : _size(), _is_set(false) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing max body size rule");
	
	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple max body size rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid max body size rule format", rule);
		if (rule.arguments[0].type != STRING)
			throw ParserTokenException("Invalid max body size argument type", rule.arguments[0]);

		_size = Size(rule);
		_is_set = true;
	}
}

std::ostream& operator<<(std::ostream& os, const MaxBodySizeRule& rule) {
	os << "MaxBodySizeRule(size: " << rule.get() << ")";
	return os;
}
