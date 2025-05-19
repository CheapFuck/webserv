#include "rules.hpp"
#include "../config.hpp"
#include "../consts.hpp"

#include <iostream>

MethodRule::MethodRule(const Object &obj, bool required) : _methods(UNKNOWN_METHOD) {
	bool found = false;

	for (const Rule &rule : obj) {
		if (rule.key != ALLOWED_METHODS) continue;

		if (rule.arguments.size() < 1)
			throw ConfigParsingException("Invalid allowed methods rule");

		for (const Argument &arg : rule.arguments) {
			if (arg.type != STRING)
				throw ConfigParsingException("Invalid method argument type");

			Method method = string_to_method(arg.str);
			if (method == UNKNOWN_METHOD)
				throw ConfigParsingException("Invalid method: " + arg.str);
			if (method & _methods)
				throw ConfigParsingException("Duplicate method: " + arg.str);
			_methods = _methods | method;
		}
	}

	if (!found) {
		if (required)
			throw ConfigParsingException("Missing allowed methods rule");
		else
			_methods = ALL_METHODS;
	}
}

inline bool MethodRule::is_allowed(Method method) const {
	return _methods & method;
}

inline const Method& MethodRule::get_methods() const {
	return _methods;
}

std::ostream& operator<<(std::ostream& os, const MethodRule& rule) {
	os << "MethodRule(methods: " << method_to_str(rule.get_methods()) << ")";
	return os;
}
