#include "rules.hpp"
#include <iostream>

MaxBodySizeRule::MaxBodySizeRule(const Object &obj, bool required) {
	int count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != MAX_BODY_SIZE) continue;

		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid max body size rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid size argument type");
		if (++count > 1)
			throw ConfigParsingException("Duplicate max body size rule");

		_size = Size(rule.arguments[0].str);
	}

	if (count == 0 && required)
		throw ConfigParsingException("Missing max body size rule");
}

inline size_t MaxBodySizeRule::get_size() const {
	return _size.get_size();
}

std::ostream& operator<<(std::ostream& os, const MaxBodySizeRule& rule) {
	os << "MaxBodySizeRule(size: " << rule.get_size() << ")";
	return os;
}
