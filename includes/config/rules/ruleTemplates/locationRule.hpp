#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include "aliasRule.hpp"
#include "methodsRule.hpp"
#include "bodyReadTimeoutRule.hpp"
#include "rootRule.hpp"
#include "uploadstoreRule.hpp"
#include "autoindexRule.hpp"
#include "indexRule.hpp"
#include "returnRule.hpp"
#include "errorpageRule.hpp"
#include "maxBodySizeRule.hpp"
#include "cgiRule.hpp"
#include "cgiTimeoutRule.hpp"
#include "cgiExtensionRule.hpp"

#include <ostream>
#include <string>

class LocationRule : public BaseRule {
private:
    bool _isSet = false;

    void _parseFromObject(Object *object);

public:
    Path path;
    MethodsRule methods;
    RootRule root;
    AliasRule alias;
    UploadStoreRule uploadStore;
    AutoIndexRule autoIndex;
    IndexRule index;
    ReturnRule returnRule;
    ErrorPageRule errorPages;
    MaxBodySizeRule maxBodySize;
    CgiRule cgi;
    CgiTimeoutRule cgiTimeout;
    CgiExtensionRule cgiExtension;
    ClientBodyReadTimeoutRule clientBodyReadTimeout;

    constexpr static Key getKey() { return Key::LOCATION; }
    constexpr static const char* getRuleName() { return "location"; }
    constexpr static const char* getRuleFormat() { return "location <path> { ... }"; }

    LocationRule(const LocationRule &other) = default;
    LocationRule& operator=(const LocationRule &other) = default;
    ~LocationRule() = default;

    LocationRule() = default;
    LocationRule(Rule *rule);
    LocationRule(Object *object);

    bool isSet() const;
};

std::ostream& operator<<(std::ostream &os, const LocationRule &rule);