#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

#define DEFAULT_CLIENT_BODY_READ_TIMEOUT 60.0

class ClientBodyReadTimeoutRule : public BaseRule {
private:
    bool _isSet = false;

public:
    Timespan timeout;

    constexpr static Key getKey() { return Key::CLIENT_BODY_TIMEOUT; }
    constexpr static const char* getRuleName() { return "client_body_timeout"; }
    constexpr static const char* getRuleFormat() { return "client_body_timeout <timeout>"; }

    ClientBodyReadTimeoutRule(const ClientBodyReadTimeoutRule &other) = default;
    ClientBodyReadTimeoutRule& operator=(const ClientBodyReadTimeoutRule &other) = default;
    ~ClientBodyReadTimeoutRule() = default;
    
    ClientBodyReadTimeoutRule();
    ClientBodyReadTimeoutRule(Rule *rule);

    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const ClientBodyReadTimeoutRule &rule);