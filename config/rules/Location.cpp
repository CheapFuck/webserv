#include "rules.hpp"

#include <unordered_map>
#include <functional>
#include <iostream>

LocationRule::LocationRule() : _path(), methods(), root(), upload_dir(), autoIndex(), index(), redirect(), errorPages(), CGI() {}

LocationRule::LocationRule(const std::string &path, Object &obj, bool throwOnNonEmpty) {
	std::unordered_map<Key, std::function<void(Rules &)>> ruleParsers = {
		{ALLOWED_METHODS, [this](const Rules &rules) { methods = MethodRule(rules, false); }},
		{ROOT, [this](const Rules &rules) { root = RootRule(rules, false); }},
		{UPLOAD_DIR, [this](const Rules &rules) { upload_dir = UploadDirRule(rules, false); }},
		{AUTOINDEX, [this](const Rules &rules) { autoIndex = AutoIndexRule(rules, false); }},
		{INDEX, [this](const Rules &rules) { index = IndexRule(rules, false); }},
		{REDIRECT, [this](const Rules &rules) { redirect = RedirectRule(rules, false); }},
		{ERROR_PAGE, [this](const Rules &rules) { errorPages = ErrorPageRule(rules, false); }},
		{CGI_PASS, [this](const Rules &rules) { CGI = CGIRule(rules, false); }},
		{CGI_TIMEOUT, [this](const Rules &rules) { cgiTimeout = CGITimeoutRule(rules, false); }}
	};

	_path = path;
	extractRules(obj, ruleParsers, throwOnNonEmpty);
}

void LocationRule::adjustFromDefault(const LocationRule &defaultLocation) {
	if (!methods.isSet() && defaultLocation.methods.isSet())
		methods = defaultLocation.methods;
	if (!root.isSet() && defaultLocation.root.isSet())
		root = defaultLocation.root;
	if (!upload_dir.isSet() && defaultLocation.upload_dir.isSet())
		upload_dir = defaultLocation.upload_dir;
	if (!autoIndex.isSet() && defaultLocation.autoIndex.isSet())
		autoIndex = defaultLocation.autoIndex;
	if (!index.isSet() && defaultLocation.index.isSet())
		index = defaultLocation.index;
	if (!redirect.isSet() && defaultLocation.redirect.isSet())
		redirect = defaultLocation.redirect;
	if (!CGI.isSet())
		CGI = defaultLocation.CGI;
	if (!cgiTimeout.isSet())
		cgiTimeout = defaultLocation.cgiTimeout;
	errorPages.updateFromDefault(defaultLocation.errorPages);
}

inline const std::string &LocationRule::getPath() const {
	return _path;
}

std::ostream& operator<<(std::ostream& os, const LocationRule& rule) {
	os << "LocationRule(path: " << rule.getPath() << ", methods: " << rule.methods << ", root: " << rule.root << ", upload_dir: " << rule.upload_dir << ", autoindex: " << rule.autoIndex << ", index: " << rule.index << ", redirect: " << rule.redirect << ", cgi_paths: " << rule.CGI << ", cgi_timeout: " << rule.cgiTimeout << ", error_pages: " << rule.errorPages << ")";
	return os;
}