#include "../ruleParser.hpp"
#include "autoindexRule.hpp"
#include "../rules.hpp"

AutoIndexRule::AutoIndexRule() :
    _isSet(false), _autoIndex(AUTO_INDEX_DEFAULT) {}

AutoIndexRule::AutoIndexRule(Rule *rule) :
    _isSet(false), _autoIndex(AUTO_INDEX_DEFAULT)
{
    if (!rule) return ;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_autoIndex);

    _isSet = true;
}

/// @brief Check if the autoindex rule is set (i.e., if it has been parsed and contains a valid value).
bool AutoIndexRule::isSet() const {
    return _isSet;
}

/// @brief Get the value of the autoindex rule.
bool AutoIndexRule::get() const {
    return _autoIndex;
}

std::ostream& operator<<(std::ostream &os, const AutoIndexRule &rule) {
    os << "AutoIndexRule: " << (rule.get() ? "on" : "off");
    return os;
}
