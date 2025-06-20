#pragma once

#include "../config.hpp"
#include "../consts.hpp"

#include <unordered_map>
#include <functional>
#include <vector>
#include <map>

#define DEFAULT_CGI_TIMEOUT 5.0

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
	PortRule();
	PortRule(const Rules &rules, bool required = true);
	int get() const;
};

class ServerNameRule {
private:
	std::string _server_name;

public:
	ServerNameRule();
	ServerNameRule(const Rules &rules, bool required = false);
	std::string get() const;
};

class MaxBodySizeRule {
private:
	Size _size;
	bool _is_set;

public:
	MaxBodySizeRule();
	MaxBodySizeRule(const Rules &rules, bool required = true);
	MaxBodySizeRule(const MaxBodySizeRule &other) = default;
	MaxBodySizeRule &operator=(const MaxBodySizeRule &other) = default;
	~MaxBodySizeRule() = default;

	inline size_t get() const { return _size.get(); }
	inline bool isSet() const { return _is_set; }
};

class CGIRule {
private:
	bool _is_set;

public:
	CGIRule();
	CGIRule(const Rules &rules, bool required = false);

	inline bool isSet() const {
		return _is_set;
	}
};

class ErrorPageRule {
private:
	std::map<int, Path> _error_pages;
	Path _default_page;

public:
	ErrorPageRule();
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
	MethodRule();
	MethodRule(const Rules &rules, bool required = false);
	MethodRule(const MethodRule &other) = default;
	MethodRule &operator=(const MethodRule &other) = default;
	~MethodRule() = default;

	bool isAllowed(Method method) const;
	const Method &getMethods() const;

	inline bool isSet() const { return _is_set; };
};

class RootRule {
private:
	Path _root;
	bool _is_set;

public:
	RootRule();
	RootRule(const Path &root);
	RootRule(const Rules &rules, bool required = false);
	RootRule(const RootRule &other) = default;
	RootRule &operator=(const RootRule &other) = default;
	 ~RootRule() = default;

	static RootRule fromGlobalRule(const RootRule &globalRule, const std::string &locationPath);

	const Path &get() const;
	bool isSet() const;
};

class IndexRule {
private:
	std::vector<std::string> _index_pages;

public:
	IndexRule();
	IndexRule(const Rules &rules, bool required = false);
	IndexRule(const IndexRule &other) =  default;
	IndexRule &operator=(const IndexRule &other) =  default;
	~IndexRule() = default;

	const std::vector<std::string> &get() const;
	bool isSet() const;
};

class AutoIndexRule {
private:
	bool _autoindex;
	bool _is_set;

public:
	AutoIndexRule();
	AutoIndexRule(const Rules &rules, bool required = false);
	AutoIndexRule(const AutoIndexRule &other) = default;
	AutoIndexRule &operator=(const AutoIndexRule &other) = default;
	~AutoIndexRule() = default;

	bool get() const;
	bool isSet() const;
};

class UploadDirRule {
private:
	Path _upload_dir;
	bool _is_set;

public:
	UploadDirRule();
	UploadDirRule(const Rules &rules, bool required = false);
	UploadDirRule(const UploadDirRule &other) = default;
	UploadDirRule &operator=(const UploadDirRule &other) = default;
	~UploadDirRule() = default;

	const Path &get() const;
	bool isSet() const;
};

class RedirectRule {
private:
	std::string _redirect;
	bool _is_set;

public:
	RedirectRule();
	RedirectRule(const Rules &rules, bool required = false);
	RedirectRule(const RedirectRule &other) = default;
 	RedirectRule &operator=(const RedirectRule &other) = default;
 	~RedirectRule() = default;

	const std::string &get() const;
	bool isSet() const;
};

class CGITimeoutRule {
private:
	double _timeout;
	bool _is_set;

public:
	CGITimeoutRule();
	CGITimeoutRule(const Rules &rules, bool required = false);
	CGITimeoutRule(const CGITimeoutRule &other) = default;
	CGITimeoutRule &operator=(const CGITimeoutRule &other) = default;
	~CGITimeoutRule() = default;

	inline double get() const { return _timeout; }
	inline bool isSet() const { return _is_set; }
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
	MaxBodySizeRule clientMaxBodySize;
	CGIRule CGI;
	CGITimeoutRule cgiTimeout;

	LocationRule();
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
	RouteRules();
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
std::ostream& operator<<(std::ostream& os, const CGITimeoutRule& rule);