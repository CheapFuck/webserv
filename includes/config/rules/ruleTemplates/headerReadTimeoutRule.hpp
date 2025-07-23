#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

#define DEFAULT_CLIENT_HEADER_READ_TIMEOUT 60.0

class ClientHeaderTimeoutRule : public BaseRule {
private:
    bool _isSet = false;

public:
    Timespan timeout;

    constexpr static Key getKey() { return Key::CLIENT_HEADER_TIMEOUT; }
    constexpr static const char* getRuleName() { return "client_header_timeout"; }
    constexpr static const char* getRuleFormat() { return "client_header_timeout <timeout>"; }

    ClientHeaderTimeoutRule(const ClientHeaderTimeoutRule &other) = default;
    ClientHeaderTimeoutRule& operator=(const ClientHeaderTimeoutRule &other) = default;
    ~ClientHeaderTimeoutRule() = default;
    
    ClientHeaderTimeoutRule();
    ClientHeaderTimeoutRule(Rule *rule);

    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const ClientHeaderTimeoutRule &rule);