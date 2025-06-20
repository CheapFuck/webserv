#include "rules.hpp"
#include "../config.hpp"

#include <iostream>

CGIRule::CGIRule() : _is_set(false) {}

CGIRule::CGIRule(const Rules &rules, bool required) : _is_set(false) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing cgi rule");
	
	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple cgi rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid cgi rule format", rule);
		if (rule.arguments[0].type != KEYWORD)
			throw ParserTokenException("Invalid cgi argument type", rule.arguments[0]);
		
		switch (rule.arguments[0].keyword) {
			case ON:
			case TRUE:
			case ENABLE:
				_is_set = true;
				break;
			case OFF:
			case FALSE:
			case DISABLE:
				_is_set = false;
				break;
			default:
				throw ParserTokenException("Invalid cgi keyword: " + rule.arguments[0].str, rule.arguments[0]);
		}
	}
}

std::ostream& operator<<(std::ostream& os, const CGIRule& rule) {
	os << "CGIRule(_is_set: " << (rule.isSet() ? "true" : "false") << ")";
	return os;
}
