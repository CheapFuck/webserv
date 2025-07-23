#include "config/rules/ruleTemplates/bodyReadTimeoutRule.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

ClientBodyReadTimeoutRule::ClientBodyReadTimeoutRule() :
    _isSet(false), timeout(DEFAULT_CLIENT_BODY_READ_TIMEOUT) {}

ClientBodyReadTimeoutRule::ClientBodyReadTimeoutRule(Rule *rule) : 
    _isSet(false), timeout(DEFAULT_CLIENT_BODY_READ_TIMEOUT)
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(timeout);

    _isSet = true;
}

/// @brief Check if the body read timeout rule is set (i.e., if it has been parsed and contains a valid timeout).
bool ClientBodyReadTimeoutRule::isSet() const {
    return _isSet;
}

std::ostream& operator<<(std::ostream &os, const ClientBodyReadTimeoutRule &rule) {
    os << "ClientBodyReadTimeoutRule: " << rule.timeout.getSeconds() << " seconds";
    return os;
}
