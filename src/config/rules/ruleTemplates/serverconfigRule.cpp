#include "config/rules/ruleTemplates/serverconfigRule.hpp"
#include "config/rules/objectParser.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

ServerConfig::ServerConfig(Rule *rule) {
    Object *object;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(object);

    ObjectParser objectParser(object);
    objectParser.bound(Key::HTTP).optional()
        .parseFromOne(clientHeaderTimeout)
        .local().required()
        .parseFromOne(port)
        .parseFromOne(serverName)
        .optional() // The routes are optional -> if none can be found the default location will be used.
        .parseRange(_locations);

    _defaultLocation = LocationRule(object);
}

/// @brief Check if the server configuration rule is set (i.e., if it contains any locations).
bool ServerConfig::isSet() const {
    return (true);
}

/// @brief Get the list of location rules defined in the server configuration.
const std::vector<LocationRule>& ServerConfig::getLocations() const {
    return _locations;
}

/// @brief Get the default location rule.
const LocationRule& ServerConfig::getDefaultLocation() const {
    return _defaultLocation;
}

/// @brief Get the location rule for a specific URL.
/// @param url The URL for which the location rule is requested.
/// @return The location rule that matches the given URL, or the default location if no specific match is found.
const LocationRule& ServerConfig::getLocation(const std::string &url) const {
    const LocationRule *bestMatch = &_defaultLocation;
    size_t longestMatch = 0;

    for (const LocationRule &location : _locations) {
        if (!location.isSet())
            continue;

        if (location.path.str().length() > longestMatch && url.starts_with(location.path.str())) {
            char charAfterMatch = url[location.path.str().length()];
            if (charAfterMatch == '\0' || charAfterMatch == '/' || charAfterMatch == '?' || location.path.str().back() == '/') {
                longestMatch = location.path.str().length();
                bestMatch = &location;
            }
        }
    }

    return (*bestMatch);
}

std::ostream& operator<<(std::ostream &os, const ServerConfig &rule) {
    os << "ServerConfig\n";
    os << "ClientHeaderTimeout: " << rule.clientHeaderTimeout << "\n";
    os << rule.port << "\n";
    os << rule.serverName << "\n";
    os << "Locations:\n";
    for (const auto &location : rule.getLocations()) {
        os << location << "\n";
    }
    os << rule.getDefaultLocation() << "\n";
    return os;
}
