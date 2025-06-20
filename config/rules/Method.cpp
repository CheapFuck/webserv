#include "rules.hpp"
#include "../config.hpp"
#include "../consts.hpp"

#include <iostream>

MethodRule::MethodRule() : _methods(UNKNOWN_METHOD), _is_set(false) {}

MethodRule::MethodRule(const Rules &rules, bool required) : _methods(UNKNOWN_METHOD), _is_set(false) {
	if (rules.empty()) {
		if (required)
			throw ParserMissingException("Missing allowed methods rule");
		else
			_methods = ALL_METHODS;
		return;
	}

	for (const Rule &rule : rules) {
		if (rule.arguments.size() < 1)
			throw ParserTokenException("Invalid allowed methods rule", rule);

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ParserTokenException("Invalid method argument type", arg);

			Method method = stringToMethod(arg.str);
			if (method == UNKNOWN_METHOD)
				throw ParserTokenException("Invalid method: " + arg.str, arg);
			if (method & _methods)
				throw ParserTokenException("Duplicate method: " + arg.str, arg);
			_methods = _methods | method;
			_is_set = true;
		}
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
