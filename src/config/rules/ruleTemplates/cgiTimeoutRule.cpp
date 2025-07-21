#include "config/rules/ruleTemplates/cgiTimeoutRule.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

CgiTimeoutRule::CgiTimeoutRule() :
    _isSet(false), timeout(CGI_DEFAULT_TIMEOUT) {}

CgiTimeoutRule::CgiTimeoutRule(Rule *rule) : 
    _isSet(false), timeout(CGI_DEFAULT_TIMEOUT)
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(timeout);

    _isSet = true;
}

/// @brief Check if the CGI timeout rule is set (i.e., if it has been parsed and contains a valid timeout).
bool CgiTimeoutRule::isSet() const {
    return _isSet;
}

std::ostream& operator<<(std::ostream &os, const CgiTimeoutRule &rule) {
    os << "CgiTimeoutRule: " << rule.timeout.getSeconds() << " seconds";
    return os;
}
