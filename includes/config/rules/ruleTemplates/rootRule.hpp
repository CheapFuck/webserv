#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class RootRule : public BaseRule {
private:
    Path _rootPath;

public:
    constexpr static Key getKey() { return Key::ROOT; }
    constexpr static const char* getRuleName() { return "root"; }
    constexpr static const char* getRuleFormat() { return "root <path>"; }

    RootRule() = default;
    RootRule(const RootRule &other) = default;
    RootRule& operator=(const RootRule &other) = default;
    ~RootRule() = default;

    RootRule(Rule *rule, const std::string &location, Object *locationObject);

    const Path &getRootPath() const;
    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const RootRule &rule);