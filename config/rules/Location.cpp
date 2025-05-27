#include "rules.hpp"

#include <iostream>

LocationRule::LocationRule(const std::string &path, const Object &obj)
	: _path(std::string(path)),
	  methods(MethodRule(obj, false)),
	  root(RootRule(obj, false)),
	  upload_dir(UploadDirRule(obj, false)),
	  autoindex(AutoIndexRule(obj, false)),
	  index(IndexRule(obj, false)),
	  redirect(RedirectRule(obj, false)),
	  cgi_paths(CGIRule(obj, false)) {}

inline const std::string &LocationRule::getPath() const {
	return _path;
}

std::ostream& operator<<(std::ostream& os, const LocationRule& rule) {
	os << "LocationRule(path: " << rule.getPath() << ", methods: " << rule.methods << ", root: " << rule.root << ", upload_dir: " << rule.upload_dir << ", autoindex: " << rule.autoindex << ", index: " << rule.index << ", redirect: " << rule.redirect << ", cgi_paths: " << rule.cgi_paths << ")";
	return os;
}