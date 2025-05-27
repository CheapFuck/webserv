#include "rules.hpp"

#include <iostream>

UploadDirRule::UploadDirRule(const Object &obj, bool required) : _is_set(false) {
	int count = 0;

	for (const Rule &rule : obj) {
		if (rule.key != UPLOAD_DIR) continue;

		if (rule.arguments.size() != 1)
			throw ConfigParsingException("Invalid upload directory rule");
		if (rule.arguments[0].type != STRING)
			throw ConfigParsingException("Invalid upload directory argument type");
		if (++count > 1)
			throw ConfigParsingException("Duplicate upload directory rule");

		_upload_dir = Path(rule.arguments[0].str);
		_is_set = true;
	}

	if (count == 0 && required)
		throw ConfigParsingException("Missing upload directory rule");
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