#include "../ruleParser.hpp"
#include "../rules.hpp"
#include "rootRule.hpp"

#include <ostream>

RootRule::RootRule(Rule *rule, const std::string &location, Object *locationObject) :
    _rootPath(Path::createDummy())
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_rootPath);

    if (rule->parentObject != locationObject)
        _rootPath.appendIgnoreAbsolute(location);
}

/// @brief Check if the root rule is set (i.e., if it has a non-empty root path).
bool RootRule::isSet() const {
    return (!_rootPath.str().empty());
}

/// @brief Get the root path specified in the rule.
const Path& RootRule::getRootPath() const {
    return _rootPath;
}

std::ostream& operator<<(std::ostream &os, const RootRule &rule) {
    os << "RootRule: " << rule.getRootPath().str();
    return os;
}
