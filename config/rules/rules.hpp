#pragma once

#include "../config.hpp"
#include "../consts.hpp"

#include <filesystem>
#include <vector>
#include <map>

class Size;
class Path;

class PortRule;
class ServerNameRule;
class MaxBodySizeRule;
class CGIRule;
class ErrorPageRule;
class MethodRule;
class RootRule;
class IndexRule;
class AutoIndexRule;
class UploadDirRule;
class RedirectRule;
class LocationRule;
class RouteRules;
class ServerConfig;

// Custom types
class Size {
private:
	size_t _size;

	void _loadSize(const std::string &str);

public:
	Size(const std::string &str);
	Size(const Size &other);
	Size();
	Size &operator=(const Size &other);

	size_t get() const;
};

class Path {
private:
	std::string _path;
	bool _is_set;
	
public:
	Path();
	Path(const Path &other);
	Path(const std::string &str);
	Path &operator=(const Path &other);

	Path &append(const std::string &str);
	Path &pop();
	Path &updateFromUrl(const std::string &route, const std::string &root);

	std::string getFilename() const;
	const std::string &str() const;

	static Path createFromUrl(const std::string &url, const LocationRule &route);
	static Path createDummy();

	bool isValid() const;
	bool exists() const;
	bool validatePermissions(std::filesystem::perms requiredPerms) const;
};

std::ostream& operator<<(std::ostream& os, const Path& path);
std::ostream& operator<<(std::ostream& os, const Size& size);

// Rules

class PortRule {
private:
	int _port;

public:
	PortRule(const Object &obj, bool required = true);
	int get() const;
};

class ServerNameRule {
private:
	std::string _server_name;

public:
	ServerNameRule(const Object &obj, bool required = false);
	std::string get() const;
};

class MaxBodySizeRule {
private:
	Size _size;

public:
	MaxBodySizeRule(const Object &obj, bool required = true);
	size_t get() const;
};

class CGIRule {
private:
	std::map<std::string, std::string> _cgi_paths;

public:
	CGIRule(const Object &obj, bool required = false);
	const std::map<std::string, std::string> &getPaths() const;
	const std::string &getPath(const std::string &ext) const;

	bool isSet() const;
	bool exists(const std::string &ext) const;
};

class ErrorPageRule {
private:
	std::map<int, Path> _error_pages;
	Path _default_page;

public:
	ErrorPageRule(const Object &obj, bool required = false);
	const Path &getErrorPage(int code) const;
	const std::map<int, Path> &getErrorPages() const;
};

class MethodRule {
private:
	Method _methods;

public:
	MethodRule(const Object &obj, bool required = false);
	bool isAllowed(Method method) const;
	const Method &getMethods() const;
};

class RootRule {
private:
	Path _root;
	bool _is_set;

public:
	RootRule(const Object &obj, bool required = false);
	const Path &get() const;
	bool isSet() const;
};

class IndexRule {
private:
	std::vector<std::string> _index_pages;

public:
	IndexRule(const Object &obj, bool required = false);
	IndexRule(const IndexRule &other);
	IndexRule &operator=(const IndexRule &other);
	const std::vector<std::string> &get() const;
	bool isSet() const;
};

class AutoIndexRule {
private:
	bool _autoindex;
	bool _is_set;

public:
	AutoIndexRule(const Object &obj, bool required = false);
	bool get() const;
	bool isSet() const;
};

class UploadDirRule {
private:
	Path _upload_dir;
	bool _is_set;

public:
	UploadDirRule(const Object &obj, bool required = false);
	const Path &get() const;
	bool isSet() const;
};

class RedirectRule {
private:
	std::string _redirect;
	bool _is_set;

public:
	RedirectRule(const Object &obj, bool required = false);
	const std::string &get() const;
	bool isSet() const;
};

class LocationRule {
private:
	std::string _path;

public:
	MethodRule methods;
	RootRule root;
	UploadDirRule upload_dir;
	AutoIndexRule autoindex;
	IndexRule index;
	RedirectRule redirect;
	CGIRule cgi_paths;

	LocationRule(const std::string &path, const Object &obj);
	const std::string &getPath() const;
};

class RouteRules {
private:
	std::vector<LocationRule> _routes;

public:
	RouteRules(const Object &obj, bool required = false);
	const std::vector<LocationRule> &getRoutes() const;
	const LocationRule *find(const std::string &url) const;
};

class ServerConfig {
public:
	PortRule port;
	ServerNameRule server_name;
	MaxBodySizeRule client_max_body_size;
	ErrorPageRule error_pages;
	RouteRules routes;

	ServerConfig(Object &object);
};

std::vector<ServerConfig> fetchServerConfigs(Object &object);

std::ostream& operator<<(std::ostream& os, const PortRule& rule);
std::ostream& operator<<(std::ostream& os, const ServerNameRule& rule);
std::ostream& operator<<(std::ostream& os, const MaxBodySizeRule& rule);
std::ostream& operator<<(std::ostream& os, const ErrorPageRule& rule);
std::ostream& operator<<(std::ostream& os, const ServerConfig& config);
std::ostream& operator<<(std::ostream& os, const LocationRule& rule);
std::ostream& operator<<(std::ostream& os, const CGIRule& rule);
std::ostream& operator<<(std::ostream& os, const RouteRules& rules);
std::ostream& operator<<(std::ostream& os, const MethodRule& rule);
std::ostream& operator<<(std::ostream& os, const RootRule& rule);
std::ostream& operator<<(std::ostream& os, const UploadDirRule& rule);
std::ostream& operator<<(std::ostream& os, const IndexRule& rule);
std::ostream& operator<<(std::ostream& os, const AutoIndexRule& rule);
std::ostream& operator<<(std::ostream& os, const RedirectRule& rule);