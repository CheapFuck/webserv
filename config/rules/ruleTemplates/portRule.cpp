#include "../ruleParser.hpp"
#include "portRule.hpp"
#include "../rules.hpp"

#include <ostream>

PortRule::PortRule() :
    _isSet(false), _port(0), _isDefault(DefaultVal(false)) {}

PortRule::PortRule(Rule *rule) :
    _isSet(false), _port(0), _isDefault(DefaultVal(false))
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1, 2)
        .parseArgument(_port)
        .parseOptionalArgument(_isDefault);

    _isSet = true;
}

/// @brief Check if the port rule is set.
bool PortRule::isSet() const {
    return _isSet;
}

/// @brief Get the port number specified in the rule.
const PortNumber& PortRule::getPort() const {
    return _port;
}

/// @brief Check if the port rule is set to default (i.e., if it has the "default" flag).
bool PortRule::isDefault() const {
    return _isDefault;
}

std::ostream& operator<<(std::ostream &os, const PortRule &rule) {
    os << "PortRule: ";
    if (rule.isSet()) {
        os << "Listening on port " << rule.getPort().value;
        if (rule.isDefault())
            os << " (default)";
    } else {
        os << "No port specified";
    }
    return os;
}
