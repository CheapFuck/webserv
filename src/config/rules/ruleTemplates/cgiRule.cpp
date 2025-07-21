#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"
#include "config/rules/ruleTemplates/cgiRule.hpp"

#include <ostream>

CgiRule::CgiRule() :
    _isSet(false), _isEnabled(CGI_DEFAULT_ENABLED) {}

CgiRule::CgiRule(Rule *rule) :
    _isSet(false), _isEnabled(CGI_DEFAULT_ENABLED)
{
    if (!rule) return ;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_isEnabled);

    _isSet = true;
}

/// @brief Check if the CGI rule is set (i.e., if it has been parsed and contains a valid path).
bool CgiRule::isSet() const {
    return _isSet;
}

/// @brief Check if the CGI rule is enabled (i.e., if it has been parsed and contains a valid enabled state).
bool CgiRule::isEnabled() const {
    return _isEnabled;
}

std::ostream& operator<<(std::ostream &os, const CgiRule &rule) {
    os << "CgiRule: " << (rule.isEnabled() ? "enabled" : "disabled");
    return os;
}