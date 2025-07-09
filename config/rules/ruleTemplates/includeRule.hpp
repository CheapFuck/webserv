#pragma once

#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class IncludeRule : public BaseRule {
private:
    std::string _includePath;

public:
    constexpr static Key getKey() { return Key::INCLUDE; }
    constexpr static const std::string getRuleName() { return "include"; }
    constexpr static const std::string getRuleFormat() { return IncludeRule::getRuleName() + " <path | name>"; }

    IncludeRule(const IncludeRule &other) = default;
    IncludeRule& operator=(const IncludeRule &other) = default;
    ~IncludeRule() = default;

    IncludeRule() = delete;
    IncludeRule(Rule *rule);

    const std::string& getIncludePath() const;
    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const IncludeRule &rule);