#include "config/rules/ruleTemplates/headerReadTimeoutRule.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

ClientHeaderTimeoutRule::ClientHeaderTimeoutRule() :
    _isSet(false), timeout(DEFAULT_CLIENT_HEADER_READ_TIMEOUT) {}

ClientHeaderTimeoutRule::ClientHeaderTimeoutRule(Rule *rule) : 
    _isSet(false), timeout(DEFAULT_CLIENT_HEADER_READ_TIMEOUT)
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(timeout);

    _isSet = true;
}

/// @brief Check if the header read timeout rule is set (i.e., if it has been parsed and contains a valid timeout).
bool ClientHeaderTimeoutRule::isSet() const {
    return _isSet;
}

std::ostream& operator<<(std::ostream &os, const ClientHeaderTimeoutRule &rule) {
    os << "ClientHeaderTimeoutRule: " << rule.timeout.getSeconds() << " seconds";
    return os;
}
