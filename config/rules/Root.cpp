#include "rules.hpp"

#include <iostream>

RootRule::RootRule(const Object &obj, bool required) : _is_set(false) {
	int count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != ROOT) continue;

		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid root rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid root argument type");
		if (++count > 1)
			throw ConfigParsingException("Duplicate root rule");

		_root = Path(rule.arguments[0].str);
		_is_set = true;
	}

	if (count == 0 && required)
		throw ConfigParsingException("Missing root rule");
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
