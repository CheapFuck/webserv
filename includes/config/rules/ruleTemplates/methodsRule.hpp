#pragma once

#include "../../types/customTypes.hpp"
#include "../../types/consts.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

#define ALLOWED_METHODS_DEFAULT (GET)

class MethodsRule : public BaseRule {
private:
    bool _isSet;
    Method _methods;

public:
    constexpr static Key getKey() { return Key::ALLOWED_METHODS; }
    constexpr static const char* getRuleName() { return "allowed_methods"; }
    constexpr static const char* getRuleFormat() { return "allowed_methods <method1> [<method2> ...]"; }

    MethodsRule(const MethodsRule &other) = default;
    MethodsRule& operator=(const MethodsRule &other) = default;
    ~MethodsRule() = default;

    MethodsRule();
    MethodsRule(const std::vector<Rule *> &rules);

    bool isAllowed(Method method) const;
    Method getMethods() const;
    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const MethodsRule &rule);
