#include "rules.hpp"

#include <iostream>

IndexRule::IndexRule() : _index_pages() {}

IndexRule::IndexRule(const Rules &rules, bool required) : _index_pages() {
	if (rules.empty() && required)
		throw ParserMissingException("Missing index rule");
	
	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple index rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() < 1)
			throw ParserTokenException("Invalid index rule format", rule);

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ParserTokenException("Invalid index argument type", arg);
			_index_pages.push_back(std::string(arg.str));
		}
	}
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
