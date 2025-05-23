#include "rules.hpp"

#include <iostream>

RouteRules::RouteRules(const Object &obj, bool required) : _routes({}) {
	int count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != LOCATION) continue;

		if (rule.arguments.size() != 2)
			throw ConfigParsingException("Invalid location rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid location path argument type");
		if (rule.arguments[1].type != OBJECT)
			throw ConfigParsingException("Invalid location object argument type");
		
		++count;
		_routes.push_back(LocationRule(rule.arguments[0].str, rule.arguments[1].rules));
	}

	if (count == 0 && required)
		throw ConfigParsingException("Missing location rule");
}

inline const std::vector<LocationRule>& RouteRules::get_routes() const {
	return _routes;
}

const LocationRule *RouteRules::find(const std::string &url) const {
	std::pair<const LocationRule *, size_t> result = std::make_pair(nullptr, 0);

	for (const LocationRule &route : _routes) {
		if (url.starts_with(route.get_path())) {
			size_t len = route.get_path().length();
			if (!(url[len] == '/' || url[len] == '\0' || url[len] == '?' || route.get_path().back() == '/')) 
				continue;
			if (!result.first || len > result.second) {
				result.first = &route;
				result.second = len;
			}
		}
	}

	return result.first;
}

std::ostream& operator<<(std::ostream& os, const RouteRules& rules) {
	os << "RouteRules(routes: [";
	for (size_t i = 0; i < rules.get_routes().size(); ++i) {
		os << rules.get_routes()[i];
		if (i < rules.get_routes().size() - 1) os << ", ";
	}
	os << "])";
	return os;
}
