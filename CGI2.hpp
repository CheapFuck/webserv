#pragma once

#include "config/rules/rules.hpp"
#include "config/consts.hpp"
#include "request.hpp"

#include <string>
#include <map>

class CGI {
private:
	std::map<std::string, std::string> _env;
	Path _documentRootPath;
	Path _scriptPath;
	Path _interpreter;

	HttpStatusCode _loadScriptPath(const Request &request, const LocationRule &route);
	HttpStatusCode _loadInterpreterPath(const LocationRule &route);

	bool _loadDocumentRootPath();

	void _setupEnvironment(const Request &request, const LocationRule &route, const ServerConfig &server);
};
