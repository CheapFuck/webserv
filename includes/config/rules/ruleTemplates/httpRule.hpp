#pragma once

#include "keepaliveReadTimeoutRule.hpp"
#include "../../types/customTypes.hpp"
#include "serverconfigRule.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class ClientHeaderTimeoutRule;

class HTTPRule : public BaseRule {
public:
	ClientHeaderTimeoutRule clientHeaderTimeout;
	ClientKeepAliveReadTimeoutRule clientKeepAliveReadTimeout;
    std::vector<ServerConfig> servers;

    constexpr static Key getKey() { return Key::HTTP; }
    constexpr static const char* getRuleName() { return "http"; }
    constexpr static const char* getRuleFormat() { return "http { ... }"; }

    HTTPRule(const HTTPRule &other) = default;
    HTTPRule& operator=(const HTTPRule &other) = default;
    ~HTTPRule() = default;

    HTTPRule();
    HTTPRule(Rule *rule);
};

std::ostream& operator<<(std::ostream &os, const HTTPRule &rule);