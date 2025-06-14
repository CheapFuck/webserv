#include "rules.hpp"

#include <iostream>

RootRule::RootRule(const Rules &rules, bool required) : _is_set(false) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing root rule");
	
	if (rules.size() > 1)
		throw ConfigParsingException("Multiple root rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid root rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid root argument type");

		_root = Path(rule.arguments[0].str);
		_is_set = true;
	}
}

inline const Path &RootRule::get() const {
	return _root;
}

inline bool RootRule::isSet() const {
	return _is_set;
}

std::ostream& operator<<(std::ostream& os, const RootRule& rule) {
	os << "RootRule(root: " << rule.get() << ", isSet: " << rule.isSet() << ")";
	return os;
}
