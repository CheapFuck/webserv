#pragma once

#include "../config.hpp"
#include "../consts.hpp"

#include <unordered_map>
#include <functional>
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
	Path(const std::string &str);
	Path(const Path &other);
	Path();
	Path &operator=(const Path &other);

	Path &append(const std::string &str);
	Path &pop();
	Path &updateFromUrl(const std::string &route, const std::string &root);
	std::string getFilename() const;
	static Path createFromUrl(const std::string &url, const LocationRule &route);
	static Path createDummy();
	const std::string &str() const;
	bool isValid() const;
};

std::ostream& operator<<(std::ostream& os, const Path& path);
std::ostream& operator<<(std::ostream& os, const Size& size);

// Rules

class PortRule {
private:
	int _port;

public:
	PortRule() = default;
	PortRule(const Rules &rules, bool required = true);
	int get() const;
};

class ServerNameRule {
private:
	std::string _server_name;

public:
	ServerNameRule() = default;
	ServerNameRule(const Rules &rules, bool required = false);
	std::string get() const;
};

class MaxBodySizeRule {
private:
	Size _size;

public:
	MaxBodySizeRule() = default;
	MaxBodySizeRule(const Rules &rules, bool required = true);
	size_t get() const;
};

class CGIRule {
private:
	std::map<std::string, std::string> _cgiPaths;

public:
	CGIRule() = default;
	CGIRule(const Rules &rules, bool required = false);
	const std::map<std::string, std::string> &getPaths() const;
	const std::string &getPath(const std::string &ext) const;

	bool isSet() const;
	bool exists(const std::string &ext) const;

	void updateFromDefault(const CGIRule &defaultRule);
};

class ErrorPageRule {
private:
	std::map<int, Path> _error_pages;
	Path _default_page;

public:
	ErrorPageRule() = default;
	ErrorPageRule(const Rules &rules, bool required = false);
	const Path &getErrorPage(int code) const;
	const std::map<int, Path> &getErrorPages() const;

	void updateFromDefault(const ErrorPageRule &defaultRule);
};

class MethodRule {
private:
	Method _methods;
	bool _is_set;

public:
	MethodRule() = default;
	MethodRule(const Rules &rules, bool required = false);
	bool isAllowed(Method method) const;
	const Method &getMethods() const;

	inline bool isSet() const { return _is_set; };
};

class RootRule {
private:
	Path _root;
	bool _is_set;

public:
	RootRule() = default;
	RootRule(const Rules &rules, bool required = false);
	const Path &get() const;
	bool isSet() const;
};

class IndexRule {
private:
	std::vector<std::string> _index_pages;

public:
	IndexRule() = default;
	IndexRule(const Rules &rules, bool required = false);
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
	AutoIndexRule() = default;
	AutoIndexRule(const Rules &rules, bool required = false);
	bool get() const;
	bool isSet() const;
};

class UploadDirRule {
private:
	Path _upload_dir;
	bool _is_set;

public:
	UploadDirRule() = default;
	UploadDirRule(const Rules &rules, bool required = false);
	const Path &get() const;
	bool isSet() const;
};

class RedirectRule {
private:
	std::string _redirect;
	bool _is_set;

public:
	RedirectRule() = default;
	RedirectRule(const Rules &rules, bool required = false);
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
	AutoIndexRule autoIndex;
	IndexRule index;
	RedirectRule redirect;
	ErrorPageRule errorPages;
	CGIRule cgiPaths;

	LocationRule() = default;
	LocationRule(const std::string &path, Object &obj, bool throwOnNonEmpty);
	LocationRule(const LocationRule &other) = default;
	LocationRule &operator=(const LocationRule &other) = default;
	~LocationRule() = default;
	
	void adjustFromDefault(const LocationRule &defaultLocation);
	const std::string &getPath() const;
};

class RouteRules {
private:
	std::vector<LocationRule> _routes;
	LocationRule _defaultLocation;

public:
	RouteRules() = default;
	RouteRules(Rules &rules, const LocationRule &defaultLocation, bool required = false);
	RouteRules(const RouteRules &other) = default;
	RouteRules &operator=(const RouteRules &other) = default;
	~RouteRules() = default;

	const std::vector<LocationRule> &getRoutes() const;
	const LocationRule *find(const std::string &url) const;
};

class ServerConfig {
public:
	PortRule port;
	ServerNameRule serverName;
	MaxBodySizeRule clientMaxBodySize;
	ErrorPageRule errorPages;
	RouteRules routes;
	LocationRule defaultLocation;

	ServerConfig() = default;
	ServerConfig(Object &object);
};

std::vector<ServerConfig> fetchServerConfigs(Object &object);
void extractRules(Object &object, const std::unordered_map<Key, std::function<void(Rules &)>> &ruleParsers, bool throwOnNonEmptyReturn);

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