#include "config/rules/ruleTemplates/uploadstoreRule.hpp"
#include "config/rules/ruleParser.hpp"
#include "config/rules/rules.hpp"

#include <ostream>
#include <string>

UploadStoreRule::UploadStoreRule()
    : _uploadDir(Path::createDummy()) {}

UploadStoreRule::UploadStoreRule(Rule *rule)
    : _uploadDir(Path::createDummy())
{
    if (!rule) return;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(_uploadDir);
}

bool UploadStoreRule::isSet() const {
    return (!_uploadDir.str().empty());
}

const Path& UploadStoreRule::getUploadDir() const {
    return _uploadDir;
}

std::ostream& operator<<(std::ostream &os, const UploadStoreRule &rule) {
    os << "UploadStoreRule: ";
    if (rule.isSet()) {
        os << "Upload Directory: " << rule.getUploadDir().str();
    } else {
        os << "Not set";
    }
    return os;
}
