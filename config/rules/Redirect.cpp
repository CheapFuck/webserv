#include "rules.hpp"

#include <iostream>

RedirectRule::RedirectRule(const Object &obj, bool required) : _is_set(false) {
	int rule_count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != REDIRECT) continue;
		if (++rule_count > 1)
			throw ConfigParsingException("Duplicate redirect rule");

		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid redirect rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid redirect argument type");

		_redirect = std::string(rule.arguments[0].str);
		_is_set = true;
	}

	if (!_is_set && required)
		throw ConfigParsingException("Missing redirect rule");
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