#include "config/rules/ruleTemplates/servernameRule.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

#include <ostream>
#include <string>

ServerNameRule::ServerNameRule()
    : _serverName("") {}

ServerNameRule::ServerNameRule(Rule *rule)
    : _serverName("")
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_serverName);
}

/// @brief Check if the server name rule is set (i.e., if it has a non-empty server name).
bool ServerNameRule::isSet() const {
    return (!_serverName.empty());
}

/// @brief Get the server name specified in the rule.
const std::string& ServerNameRule::getServerName() const {
    return _serverName;
}

std::ostream& operator<<(std::ostream &os, const ServerNameRule &rule) {
    os << "ServerNameRule: ";
    if (rule.isSet()) {
        os << "Server Name: " << rule.getServerName();
    } else {
        os << "Not set";
    }
    return os;
}
