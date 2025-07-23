#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

#define DEFAULT_CLIENT_KEEPALIVE_READ_TIMEOUT 75.0

class ClientKeepAliveReadTimeoutRule : public BaseRule {
private:
    bool _isSet = false;

public:
    Timespan timeout;

    constexpr static Key getKey() { return Key::CLIENT_KEEPALIVE_READ_TIMEOUT; }
    constexpr static const char* getRuleName() { return "keepalive_timeout"; }
    constexpr static const char* getRuleFormat() { return "keepalive_timeout <timeout>"; }

    ClientKeepAliveReadTimeoutRule(const ClientKeepAliveReadTimeoutRule &other) = default;
    ClientKeepAliveReadTimeoutRule& operator=(const ClientKeepAliveReadTimeoutRule &other) = default;
    ~ClientKeepAliveReadTimeoutRule() = default;
    
    ClientKeepAliveReadTimeoutRule();
    ClientKeepAliveReadTimeoutRule(Rule *rule);

    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const ClientKeepAliveReadTimeoutRule &rule);