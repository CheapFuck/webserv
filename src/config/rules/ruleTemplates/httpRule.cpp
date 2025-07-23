#include "config/rules/ruleTemplates/httpRule.hpp"
#include "config/types/customTypes.hpp"
#include "config/rules/objectParser.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

#include <ostream>

HTTPRule::HTTPRule() : clientHeaderTimeout(), servers({}) {}

HTTPRule::HTTPRule(Rule *rule) {
    Object *object;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(object);

    ObjectParser objectParser(object);
	objectParser.local().optional()
		.parseFromOne(clientHeaderTimeout)
		.parseFromOne(clientKeepAliveReadTimeout)
		.required()
		.parseRange(servers);
}

std::ostream& operator<<(std::ostream &os, const HTTPRule &rule) {
    os << "HTTPRule: ";
    os << "Client Header Timeout: " << rule.clientHeaderTimeout << "\n";
	os << "Servers:\n";
	for (const auto &server : rule.servers)
		os << server << "\n";
    return os;
}
