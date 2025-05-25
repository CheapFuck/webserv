#pragma once

#include "../config.hpp"
#include "../consts.hpp"

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

	void _load_size(const std::string &str);

public:
	Size(const std::string &str);
	Size();

	size_t get() const;
};

class Path {
private:
	std::string _path;
	bool _is_set;
	
public:
	Path(const std::string &str);
	Path(const Path &other);
	Path();
	Path &operator=(const Path &other);

	Path &append(const std::string &str);
	Path &pop();
	Path &update_from_url(const std::string &route, const std::string &root);
	std::string get_filename() const;
	static Path create_from_url(const std::string &url, const LocationRule &route);
	static Path create_dummy();
	const std::string &get_path() const;
	bool is_valid() const;
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
	const std::map<std::string, std::string> &get_paths() const;
	const std::string &get_path(const std::string &ext) const;

	bool is_set() const;
	bool exists(const std::string &ext) const;
};

class ErrorPageRule {
private:
	std::map<int, Path> _error_pages;
	Path _default_page;

public:
	ErrorPageRule(const Object &obj, bool required = false);
	const Path &get_error_page(int code) const;
	const std::map<int, Path> &get_error_pages() const;
};

class MethodRule {
private:
	Method _methods;

public:
	MethodRule(const Object &obj, bool required = false);
	bool is_allowed(Method method) const;
	const Method &get_methods() const;
};

class RootRule {
private:
	Path _root;
	bool _is_set;

public:
	RootRule(const Object &obj, bool required = false);
	const Path &get() const;
	bool is_set() const;
};

class IndexRule {
private:
	std::vector<std::string> _index_pages;

public:
	IndexRule(const Object &obj, bool required = false);
	const std::vector<std::string> &get() const;
	bool is_set() const;
};

class AutoIndexRule {
private:
	bool _autoindex;
	bool _is_set;

public:
	AutoIndexRule(const Object &obj, bool required = false);
	bool get() const;
	bool is_set() const;
};

class UploadDirRule {
private:
	Path _upload_dir;
	bool _is_set;

public:
	UploadDirRule(const Object &obj, bool required = false);
	const Path &get() const;
	bool is_set() const;
};

class RedirectRule {
private:
	std::string _redirect;
	bool _is_set;

public:
	RedirectRule(const Object &obj, bool required = false);
	const std::string &get() const;
	bool is_set() const;
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
	const std::string &get_path() const;
};

class RouteRules {
private:
	std::vector<LocationRule> _routes;

public:
	RouteRules(const Object &obj, bool required = false);
	const std::vector<LocationRule> &get_routes() const;
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

std::vector<ServerConfig> fetch_server_configs(Object &object);

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