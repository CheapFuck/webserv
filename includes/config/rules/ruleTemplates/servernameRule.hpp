#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class ServerNameRule : public BaseRule {
private:
    std::string _serverName;

public:
    constexpr static Key getKey() { return Key::SERVER_NAME; }
    constexpr static const char* getRuleName() { return "server_name"; }
    constexpr static const char* getRuleFormat() { return "server_name <name> [default]"; }

    ServerNameRule(const ServerNameRule &other) = default;
    ServerNameRule& operator=(const ServerNameRule &other) = default;
    ~ServerNameRule() = default;

    ServerNameRule();
    ServerNameRule(Rule *rule);

    bool isSet() const;
    const std::string& getServerName() const;
};

std::ostream& operator<<(std::ostream &os, const ServerNameRule &rule);
