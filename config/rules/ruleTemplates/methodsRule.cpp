#include "../ruleParser.hpp"
#include "methodsRule.hpp"
#include "../rules.hpp"

#include <ostream>
#include <vector>

MethodsRule::MethodsRule() :
    _isSet(false), _methods(ALLOWED_METHODS_DEFAULT) {}

MethodsRule::MethodsRule(const std::vector<Rule *> &rules) :
    _isSet(false), _methods(Method::UNKNOWN_METHOD)
{
    for (Rule *rule : rules) {
        std::vector<Method> methods;

        RuleParser::create(rule, *this)
            .expectMinNumArguments(1)
            .parseAll(methods);

        for (const Method &method : methods) {
            _methods = method | _methods;
            _isSet = true;
        }
    }

    if (!_isSet)
        _methods = ALLOWED_METHODS_DEFAULT;
}

/// @brief Check if the specified method is allowed by this rule.
bool MethodsRule::isAllowed(Method method) const {
    return (_methods & method) != Method::UNKNOWN_METHOD;
}

/// @brief Get the methods defined by this rule.
Method MethodsRule::getMethods() const {
    return _methods;
}

/// @brief Check if the methods rule is set (i.e., if any methods have been defined).
bool MethodsRule::isSet() const {
    return _isSet;
}

std::ostream& operator<<(std::ostream &os, const MethodsRule &rule) {
    os << "MethodsRule: ";
    if (rule.isSet()) {
        os << "Allowed methods: " << rule.getMethods();
    } else {
        os << "No methods allowed";
    }
    return os;
}
