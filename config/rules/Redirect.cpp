#include "../../print.hpp"
#include "rules.hpp"

#include <iostream>

RedirectRule::RedirectRule() : _redirect(), _is_set(false) {}

RedirectRule::RedirectRule(const Rules &rules, bool required) : _redirect(), _is_set(false) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing redirect rule");
	
	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple redirect rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid redirect rule format", rule);
		if (rule.arguments[0].type != STRING)
			throw ParserTokenException("Invalid redirect argument type", rule.arguments[0]);

		_redirect = std::string(rule.arguments[0].str);
		_is_set = true;
	}
}

/// @brief Get the redirect URL
/// @return Either the redirect URL or an empty string if not set
inline const std::string &RedirectRule::get() const {
	return _redirect;
}

/// @brief Check if the redirect rule is set
/// @return True if the redirect rule is set, false otherwise
inline bool RedirectRule::isSet() const {
	return _is_set;
}

std::ostream& operator<<(std::ostream& os, const RedirectRule& rule) {
	os << "RedirectRule(redirect: " << rule.get() << ", isSet: " << rule.isSet() << ")";
	return os;
}