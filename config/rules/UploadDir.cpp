#include "rules.hpp"

#include <iostream>

UploadDirRule::UploadDirRule() : _upload_dir(Path::createDummy()), _is_set(false) {}

UploadDirRule::UploadDirRule(const Rules &rules, bool required) : _is_set(false) {
	if (rules.empty() && required)
		throw ParserMissingException("Missing upload directory rule");

	if (rules.size() > 1)
		throw ParserDuplicationException("Multiple upload directory rules found", rules[0], rules[1]);

	for (const Rule &rule : rules) {
		if (rule.arguments.size() != 1)
			throw ParserTokenException("Invalid upload directory rule format", rule);
		if (rule.arguments[0].type != STRING)
			throw ParserTokenException("Invalid upload directory argument type", rule.arguments[0]);

		_upload_dir = Path(rule.arguments[0].str);
		_is_set = true;
	}
}

inline const Path &UploadDirRule::get() const {
	return _upload_dir;
}

inline bool UploadDirRule::isSet() const {
	return _is_set;
}

std::ostream& operator<<(std::ostream& os, const UploadDirRule& rule) {
	os << "UploadDirRule(upload_dir: " << rule.get() << ", isSet: " << rule.isSet() << ")";
	return os;
}