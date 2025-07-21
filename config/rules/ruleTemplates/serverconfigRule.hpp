#pragma once

#include "../../types/customTypes.hpp"
#include "servernameRule.hpp"
#include "locationRule.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"
#include "portRule.hpp"

#include <string>

class ServerConfig : public BaseRule {
private:
    std::vector<LocationRule> _locations;
    LocationRule _defaultLocation;


public:
    PortRule port;
    ServerNameRule serverName;

    constexpr static Key getKey() { return Key::SERVER; }
    constexpr static const char* getRuleName() { return "server"; }
    constexpr static const char* getRuleFormat() { return "server { ... }"; }

    ServerConfig() = default;
    ServerConfig(Rule *rule);

    bool isSet() const;
    const std::vector<LocationRule>& getLocations() const;
    const LocationRule& getDefaultLocation() const;
    const LocationRule& getLocation(const std::string &url) const;
};

std::ostream& operator<<(std::ostream &os, const ServerConfig &rule);
