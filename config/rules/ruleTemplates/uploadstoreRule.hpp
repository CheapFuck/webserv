#pragma once

#include "../../types/customTypes.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"

#include <ostream>
#include <string>

class UploadStoreRule : public BaseRule {
private:
    Path _uploadDir;

public:
    constexpr static Key getKey() { return Key::UPLOAD_DIR; }
    constexpr static const std::string getRuleName() { return "upload_store"; }
    constexpr static const std::string getRuleFormat() { return UploadStoreRule::getRuleName() + " <path>"; }

    UploadStoreRule(const UploadStoreRule &other) = default;
    UploadStoreRule& operator=(const UploadStoreRule &other) = default;
    ~UploadStoreRule() = default;

    UploadStoreRule();
    UploadStoreRule(Rule *rule);

    bool isSet() const;
    const Path& getUploadDir() const;
};

std::ostream& operator<<(std::ostream &os, const UploadStoreRule &rule);