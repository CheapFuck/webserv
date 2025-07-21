#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class AliasRule : public BaseRule {
private:
    Path _aliasPath;

public:
    constexpr static Key getKey() { return Key::ALIAS; }
    constexpr static const char* getRuleName() { return "alias"; }
    constexpr static const char* getRuleFormat() { return "alias <path>"; }

    AliasRule(const AliasRule &other) = default;
    AliasRule& operator=(const AliasRule &other) = default;
    ~AliasRule() = default;

    AliasRule();
    AliasRule(Rule *rule);

    bool isSet() const;
    const Path &getAliasPath() const;
};

std::ostream& operator<<(std::ostream &os, const AliasRule &rule);
