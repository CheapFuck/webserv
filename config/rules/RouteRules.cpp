#include "rules.hpp"

#include <iostream>

RouteRules::RouteRules() : _routes(), _defaultLocation() {}

RouteRules::RouteRules(Rules &rules, const LocationRule &defaultLocation, bool required) {
	_defaultLocation = defaultLocation;

	if (rules.empty() && required)
		throw ParserMissingException("Missing location rules");

	for (Rule &rule : rules) {
		if (rule.arguments.size() != 2)
			throw ParserTokenException("Invalid location rule format", rule);
		if (rule.arguments[0].type != STRING)
			throw ParserTokenException("Invalid location path argument type", rule.arguments[0]);
		if (rule.arguments[1].type != OBJECT)
			throw ParserTokenException("Invalid location rules argument type", rule.arguments[1]);
		_routes.push_back(LocationRule(rule.arguments[0].str, rule.arguments[1].rules, true));
		_routes.back().adjustFromDefault(defaultLocation);
	}
}

inline const std::vector<LocationRule>& RouteRules::getRoutes() const {
	return _routes;
}

/// @brief Find the longest matching route for the given URL.
/// @param url The URL to match against the routes.
/// @return A pointer to the matching LocationRule, or nullptr if no match is found.
const LocationRule *RouteRules::find(const std::string &url) const {
	std::pair<const LocationRule *, size_t> result = std::make_pair(nullptr, 0);

	for (const LocationRule &route : _routes) {
		if (url.starts_with(route.getPath())) {
			size_t len = route.getPath().length();
			if (!(url[len] == '/' || url[len] == '\0' || url[len] == '?' || route.getPath().back() == '/')) 
				continue;
			if (!result.first || len > result.second) {
				result.first = &route;
				result.second = len;
			}
		}
	}

	if (!result.first)
		return (&_defaultLocation);
	return (result.first);
}

std::ostream& operator<<(std::ostream& os, const RouteRules& rules) {
	os << "RouteRules(routes: [";
	for (size_t i = 0; i < rules.getRoutes().size(); ++i) {
		os << rules.getRoutes()[i];
		if (i < rules.getRoutes().size() - 1) os << ", ";
	}
	os << "])";
	return os;
}
