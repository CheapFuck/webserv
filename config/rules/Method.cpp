#include "rules.hpp"
#include "../config.hpp"
#include "../consts.hpp"

#include <iostream>

MethodRule::MethodRule() : _methods(UNKNOWN_METHOD), _is_set(false) {}

MethodRule::MethodRule(const Rules &rules, bool required) : _methods(UNKNOWN_METHOD) {
	bool found = false;

	for (const Rule &rule : rules) {
		if (rule.arguments.size() < 1)
			throw ConfigParsingException("Invalid allowed methods rule");

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ConfigParsingException("Invalid method argument type");

			Method method = stringToMethod(arg.str);
			if (method == UNKNOWN_METHOD)
				throw ConfigParsingException("Invalid method: " + arg.str);
			if (method & _methods)
				throw ConfigParsingException("Duplicate method: " + arg.str);
			_methods = _methods | method;
			found = true;
		}
	}

	if (!found) {
		if (required)
			throw ConfigParsingException("Missing allowed methods rule");
		else
			_methods = ALL_METHODS;
	}
}

/// @brief Check if a method is allowed by this rule
/// @param method The HTTP method to check
bool MethodRule::isAllowed(Method method) const {
	return _methods & method;
}

const Method& MethodRule::getMethods() const {
	return _methods;
}

std::ostream& operator<<(std::ostream& os, const MethodRule& rule) {
	os << "MethodRule(methods: " << methodToStr(rule.getMethods()) << ")";
	return os;
}
