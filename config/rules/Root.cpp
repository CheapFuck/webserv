#include "rules.hpp"
#include "../../print.hpp"

#include <iostream>

RootRule::RootRule() : _root(), _is_set(false) {}

RootRule::RootRule(const Path &root) : _root(root), _is_set(true) {}

RootRule::RootRule(const Rules &rules, bool required) : _root(), _is_set(false) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing root rule");
	
	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple root rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid root rule format", rule);
		if (rule.arguments[0].type != STRING)
			throw ParserTokenException("Invalid root argument type", rule.arguments[0]);

		_root = Path(rule.arguments[0].str);
		_is_set = true;
	}
}

/// @brief  Create a RootRule from a global rule and a location path.
/// @param globalRule The global RootRule to base the new rule on.
/// @param locationPath The location path to append to the global root.
/// @return A new RootRule constructed from the global rule and location path.
RootRule RootRule::fromGlobalRule(const RootRule &globalRule, const std::string &locationPath) {
	if (globalRule.isSet()) {
		Path globalRoot = globalRule.get();
		std::string trimmedPath = locationPath;
		trimmedPath.erase(0, trimmedPath.find_first_not_of('/'));
		globalRoot.append(trimmedPath);
		return RootRule(globalRoot);
	}
	return RootRule();
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
