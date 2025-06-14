#include "rules.hpp"

#include <iostream>

IndexRule::IndexRule(const Rules &rules, bool required) {
	if (rules.empty() && required)
		throw ConfigParsingException("Missing index rule");
	
	if (rules.size() > 1)
		throw ConfigParsingException("Multiple index rules found");

	for (const Rule &rule : rules) {
		if (rule.arguments.size() < 1)
			throw ConfigParsingException("Invalid index rule");

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ConfigParsingException("Invalid index argument type");
			_index_pages.push_back(std::string(arg.str));
		}
	}
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
