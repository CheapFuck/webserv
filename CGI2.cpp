#include "config/rules/rules.hpp"
#include "config/consts.hpp"
#include "response.hpp"
#include "request.hpp"
#include "Utils.hpp"
#include "print.hpp"
#include "CGI2.hpp"

#include <unistd.h>
#include <limits.h>
#include <string>
#include <map>


HttpStatusCode CGI::_loadScriptPath(const Request &request, const LocationRule &route) {
	_scriptPath = _documentRootPath;
	_scriptPath.append(Path::createFromUrl(request.metadata.getPath(), route).str());

	if (!_scriptPath.isValid()) {
		DEBUG("CGI: Invalid script path: " << _scriptPath.str());
		return (HttpStatusCode::BadRequest);
	}

	if (!_scriptPath.exists()) {
		DEBUG("CGI: Script path does not exist: " << _scriptPath.str());
		return (HttpStatusCode::NotFound);
	}

	if (!_scriptPath.validatePermissions(std::filesystem::perms::owner_exec)) {
		DEBUG("CGI: Script path does not have execute permissions: " << _scriptPath.str());
		return (HttpStatusCode::InternalServerError);
	}

	DEBUG("CGI: Script path loaded: " << _scriptPath.str());
}

HttpStatusCode CGI::_loadInterpreterPath(const LocationRule &route) {
	std::string extention = Utils::getFileExtension(_scriptPath.getFilename());
	if (!route.cgi_paths.exists(extention)) {
		DEBUG("CGI: No CGI interpreter found for extension: " << extention);
		return (HttpStatusCode::NotImplemented);
	}

	_interpreter = _documentRootPath;
	_interpreter.append(route.cgi_paths.getPath(extention));

	if (!_interpreter.exists()) {
		DEBUG("CGI: Interpreter path does not exist: " << _interpreter.str());
		return (HttpStatusCode::InternalServerError);
	}

	if (!_interpreter.validatePermissions(std::filesystem::perms::owner_exec)) {
		DEBUG("CGI: Interpreter path does not have execute permissions: " << _interpreter.str());
		return (HttpStatusCode::InternalServerError);
	}

	DEBUG("CGI: Interpreter path loaded: " << _interpreter.str());
}

bool CGI::_loadDocumentRootPath() {
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == nullptr) {
		ERROR("CGI: getcwd() error");
		return (false);
	}
	_documentRootPath = Path(std::string(cwd));
	return (true);
}

/// @brief Sets up the CGI environment variables based on the request, route, and server configuration.
/// @details https://www6.uniovi.es/~antonio/ncsa_httpd/cgi/env.html
/// @param request The HTTP request containing metadata and headers.
/// @param route The location rule that matches the request path.
/// @param server The server configuration containing server name and port.
void CGI::_setupEnvironment(const Request &request, const LocationRule &route, const ServerConfig &server) {
	_env.clear();

	_env["SERVER_SOFTWARE"] = "webserv/1.0";
	_env["SERVER_NAME"] = server.server_name.get();
	_env["GATEWAY_INTERFACE"] = Response::tlsVersion;
	_env["SERVER_PROTOCOL"] = Response::protocol;
	_env["SERVER_PORT"] = Utils::intToString(server.port.get());
	_env["REQUEST_METHOD"] = methodToStr(request.metadata.getMethod());
	_env["PATH_INFO"] = 



	_env["SCRIPT_FILENAME"] = _scriptPath.str();
	_env["REQUEST_METHOD"] = methodToStr(request.metadata.getMethod());
	_env["SCRIPT_NAME"] = request.metadata.getPath();
	_env["PATH_INFO"] = request.metadata.getPath();
	_env["PATH_TRANSLATED"] = _scriptPath.str();
	_env["REQUEST_URI"] = request.metadata.getPath();
	_env["SERVER_NAME"] = server.server_name.get().empty() ? "Whatever" : server.server_name.get();
	_env["SERVER_PORT"] = Utils::intToString(server.port.get());
	_env["SERVER_PROTOCOL"] = Response::protocol;
	_env["SERVER_SOFTWARE"] = "webserv/1.0";
	_env["GATEWAY_INTERFACE"] = Response::tlsVersion;

	size_t queryPos = request.metadata.getPath().find('?');
	if (queryPos != std::string::npos) {
		_env["QUERY_STRING"] = request.metadata.getPath().substr(queryPos + 1);
	} else {
		_env["QUERY_STRING"] = "";
	}

	DEBUG("CGI: Environment variables set up successfully");
}