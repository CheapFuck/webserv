#include "maxBodySizeRule.hpp"
#include "../ruleParser.hpp"
#include "../rules.hpp"

#include <ostream>

MaxBodySizeRule::MaxBodySizeRule() :
    _isSet(false), _maxBodySize(Size(0)) {}

MaxBodySizeRule::MaxBodySizeRule(Rule *rule) :
    _isSet(false), _maxBodySize(Size(0))
{
    if (!rule) return ;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_maxBodySize);

    _isSet = true;
}

/// @brief Check if the max body size rule is set (i.e., if a maximum body size has been defined).
bool MaxBodySizeRule::isSet() const {
    return _isSet;
}

/// @brief Get the maximum body size defined by this rule.
const Size& MaxBodySizeRule::getMaxBodySize() const {
    return _maxBodySize;
}

std::ostream& operator<<(std::ostream &os, const MaxBodySizeRule &rule) {
    os << MaxBodySizeRule::getRuleName() << " " << rule.getMaxBodySize();
    return os;
}
