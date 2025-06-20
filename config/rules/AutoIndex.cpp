#include "rules.hpp"

#include <iostream>

AutoIndexRule::AutoIndexRule() : _autoindex(false), _is_set(false) {}

AutoIndexRule::AutoIndexRule(const Rules &rules, bool required) : _autoindex(false), _is_set(false) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing autoindex rule");

	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple autoindex rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid autoindex rule format", rule);
		if (rule.arguments[0].type != KEYWORD)
			throw ParserTokenException("Invalid autoindex argument type", rule.arguments[0]);

		switch (rule.arguments[0].keyword) {
			case ON:
			case TRUE:
			case ENABLE:
				_autoindex = true;
				_is_set = true;
				break;
			case OFF:
			case FALSE:
			case DISABLE:
				_autoindex = false;
				_is_set = true;
				break;
			default:
				throw ParserTokenException("Invalid autoindex keyword: " + rule.arguments[0].str, rule.arguments[0]);
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
