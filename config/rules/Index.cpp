#include "rules.hpp"

#include <iostream>

IndexRule::IndexRule(const Object &obj, bool required) {
	int rule_count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != INDEX) continue;
		if (++rule_count > 1)
			throw ConfigParsingException("Duplicate index rule");

		if (rule.arguments.size() < 1)
			throw ConfigParsingException("Invalid index rule");

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ConfigParsingException("Invalid index argument type");
			_index_pages.push_back(std::string(arg.str));
		}
	}

	if (_index_pages.empty() && required)
		throw ConfigParsingException("Missing index rule");
}

IndexRule::IndexRule(const IndexRule &other) : _index_pages(other._index_pages) {}

IndexRule &IndexRule::operator=(const IndexRule &other) {
	if (this != &other) {
		_index_pages = other._index_pages;
	}
	return *this;
}

inline const std::vector<std::string> &IndexRule::get() const {
	return _index_pages;
}

inline bool IndexRule::isSet() const {
	return !_index_pages.empty();
}

std::ostream& operator<<(std::ostream& os, const IndexRule& rule) {
	if (!rule.isSet()) {
		os << "IndexRule(isSet: false)";
		return os;
	}

	os << "IndexRule(pages: ";
	std::vector<std::string>::const_iterator it = rule.get().begin();
	while (it != rule.get().end() && it != rule.get().end() - 1) {
		os << *it;
		++it;
		if (it != rule.get().end())
			os << ", ";
	}
	os << ")";
	return os;
}
