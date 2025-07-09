#pragma once

#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

#define AUTO_INDEX_DEFAULT false

class AutoIndexRule : public BaseRule {
private:
    bool _isSet;
    bool _autoIndex;

public:
    constexpr static Key getKey() { return Key::AUTOINDEX; }
    constexpr static const std::string getRuleName() { return "autoindex"; }
    constexpr static const std::string getRuleFormat() { return AutoIndexRule::getRuleName() + " <on|off>"; }

    AutoIndexRule(const AutoIndexRule &other) = default;
    AutoIndexRule& operator=(const AutoIndexRule &other) = default;
    ~AutoIndexRule() = default;
    
    AutoIndexRule();
    AutoIndexRule(Rule *rule);

    bool isSet() const;
    bool get() const;
};

std::ostream& operator<<(std::ostream &os, const AutoIndexRule &rule);