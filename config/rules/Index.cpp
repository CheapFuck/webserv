#include "rules.hpp"

#include <iostream>

IndexRule::IndexRule(const Object &obj, bool required) : _is_set(false) {
	int rule_count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != INDEX) continue;
		if (++rule_count > 1)
			throw ConfigParsingException("Duplicate index rule");

		if (rule.arguments.size() < 1)
			throw ConfigParsingException("Invalid index rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid index argument type");

		_index = std::string(rule.arguments[0].str);
		_is_set = true;
	}

	if (!_is_set && required)
		throw ConfigParsingException("Missing index rule");
}

inline const std::string &IndexRule::get() const {
	return _index;
}

inline bool IndexRule::is_set() const {
	return _is_set;
}

std::ostream& operator<<(std::ostream& os, const IndexRule& rule) {
	os << "IndexRule(index: " << rule.get() << ", is_set: " << rule.is_set() << ")";
	return os;
}
