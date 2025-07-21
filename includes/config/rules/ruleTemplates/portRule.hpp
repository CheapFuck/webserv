#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class PortRule : public BaseRule {
private:
    bool _isSet;
    PortNumber _port;
    DefaultVal _isDefault;

public:
    constexpr static Key getKey() { return Key::LISTEN; }
    constexpr static const char* getRuleName() { return "listen"; }
    constexpr static const char* getRuleFormat() { return "listen <port> [default]"; }

    PortRule(const PortRule &other) = default;
    PortRule& operator=(const PortRule &other) = default;
    ~PortRule() = default;

    PortRule();
    PortRule(Rule *rule);

    bool isSet() const;
    bool isDefault() const;
    const PortNumber& getPort() const;
};

std::ostream& operator<<(std::ostream &os, const PortRule &rule);
