#include "rules.hpp"
#include "../config.hpp"

#include <iostream>

CGIRule::CGIRule() : _is_set(false) {}

CGIRule::CGIRule(const Rules &rules, bool required) : _is_set(false) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing cgi rule");
	
	if (rules.size() > 1)
		throw ConfigParsingException("Multiple cgi rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid cgi_pass rule");
		if (rule.arguments[0].type != KEYWORD)
			throw ConfigParsingException("Invalid cgi_pass argument type");
		
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
				throw ConfigParsingException("Invalid cgi_pass keyword: " + rule.arguments[0].str);
		}
	}
}

std::ostream& operator<<(std::ostream& os, const CGIRule& rule) {
	os << "CGIRule(_is_set: " << (rule.isSet() ? "true" : "false") << ")";
	return os;
}
