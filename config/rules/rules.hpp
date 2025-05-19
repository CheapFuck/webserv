#pragma once

#include "../config.hpp"
#include "../consts.hpp"

#include "../custom_types/types.hpp"

class PortRule {
private:
	int _port;

public:
	PortRule(const Object &obj, bool required = true);
	inline int get() const;
};

class ServerNameRule {
private:
	std::string _server_name;

public:
	ServerNameRule(const Object &obj, bool required = false);
	inline std::string get() const;
};

class MaxBodySizeRule {
private:
	Size _size;

public:
	MaxBodySizeRule(const Object &obj, bool required = true);
	inline size_t get_size() const;
};

class ErrorPageRule {
private:
	std::map<int, Path> _error_pages;
	Path _default_page;

public:
	ErrorPageRule(const Object &obj, bool required = false);
	const Path &get_error_page(int code) const;
	inline const std::map<int, Path> &get_error_pages() const;
};

class MethodRule {
private:
	Method _methods;

public:
	MethodRule(const Object &obj, bool required = false);
	inline bool is_allowed(Method method) const;
	inline const Method &get_methods() const;
};

class RootRule {
private:
	Path _root;
	bool _is_set;

public:
	RootRule(const Object &obj, bool required = false);
	inline const Path &get() const;
	inline bool is_set() const;
};

class IndexRule {
private:
	std::string _index;
	bool _is_set;

public:
	IndexRule(const Object &obj, bool required = false);
	inline const std::string &get() const;
	inline bool is_set() const;
};

class AutoIndexRule {
private:
	bool _autoindex;
	bool _is_set;

public:
	AutoIndexRule(const Object &obj, bool required = false);
	inline bool get() const;
	inline bool is_set() const;
};

class UploadDirRule {
private:
	Path _upload_dir;
	bool _is_set;

public:
	UploadDirRule(const Object &obj, bool required = false);
	inline const Path &get() const;
	inline bool is_set() const;
};

class RedirectRule {
private:
	std::string _redirect;
	bool _is_set;

public:
	RedirectRule(const Object &obj, bool required = false);
	inline const std::string &get() const;
	inline bool is_set() const;
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

	LocationRule(const std::string &path, const Object &obj);
	inline const std::string &get_path() const;
};

class RouteRules {
private:
	std::vector<LocationRule> _routes;

public:
	RouteRules(const Object &obj, bool required = false);
	inline const std::vector<LocationRule> &get_routes() const;
	inline const LocationRule &get_route(std::string &url) const;
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
std::ostream& operator<<(std::ostream& os, const RouteRules& rules);
std::ostream& operator<<(std::ostream& os, const MethodRule& rule);
std::ostream& operator<<(std::ostream& os, const RootRule& rule);
std::ostream& operator<<(std::ostream& os, const UploadDirRule& rule);
std::ostream& operator<<(std::ostream& os, const IndexRule& rule);
std::ostream& operator<<(std::ostream& os, const AutoIndexRule& rule);
std::ostream& operator<<(std::ostream& os, const RedirectRule& rule);