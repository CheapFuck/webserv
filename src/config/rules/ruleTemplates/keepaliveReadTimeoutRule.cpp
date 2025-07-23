#include "config/rules/ruleTemplates/keepaliveReadTimeoutRule.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

ClientKeepAliveReadTimeoutRule::ClientKeepAliveReadTimeoutRule() :
    _isSet(false), timeout(DEFAULT_CLIENT_KEEPALIVE_READ_TIMEOUT) {}

ClientKeepAliveReadTimeoutRule::ClientKeepAliveReadTimeoutRule(Rule *rule) : 
    _isSet(false), timeout(DEFAULT_CLIENT_KEEPALIVE_READ_TIMEOUT)
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(timeout);

    _isSet = true;
}

/// @brief Check if the CGI timeout rule is set (i.e., if it has been parsed and contains a valid timeout).
bool ClientKeepAliveReadTimeoutRule::isSet() const {
    return _isSet;
}

std::ostream& operator<<(std::ostream &os, const ClientKeepAliveReadTimeoutRule &rule) {
    os << "ClientKeepAliveReadTimeoutRule: " << rule.timeout.getSeconds() << " seconds";
    return os;
}
