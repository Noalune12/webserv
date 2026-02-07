#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include "Connection.hpp"
#include "CGIExecutor.hpp"
#include "Logger.hpp"

bool	CGIExecutor::verifyCGIPath(const std::string& path, Connection& client) {

	std::string	root = client._request._reqLocation->root.empty() ? client._request._reqLocation->alias : client._request._reqLocation->root;

	if (access(path.c_str(), F_OK) != 0) {
		Logger::error("CGI interpreter not found: " + path);
		client._request.err = true;
		client._request.status = 500;
		client._request.findErrorPage(500, root, client._request._reqLocation->errPage);
		return (false);
	}

	struct stat	st;

	if (stat(path.c_str(), &st) != 0) {
		Logger::error("Cannot stat CGI interpreter: " + path);
		client._request.err = true;
		client._request.status = 500;
		client._request.findErrorPage(500, root, client._request._reqLocation->errPage);
		return (false);
	}

	if (!S_ISREG(st.st_mode)) {
		Logger::error("CGI interpreter is not a regular file: " + path);
		client._request.err = true;
		client._request.status = 500;
		client._request.findErrorPage(500, root, client._request._reqLocation->errPage);
		return (false);
	}

	if (access(path.c_str(), X_OK) != 0) {
		Logger::error("CGI interpreter is not executable: " + path);
		client._request.err = true;
		client._request.status = 500;
		client._request.findErrorPage(500, root, client._request._reqLocation->errPage);
		return (false);
	}

	return (true);
}

bool	CGIExecutor::verifyCGIScript(const std::string& scriptPath, Connection& client) {

	std::string	root = client._request._reqLocation->root.empty() ? client._request._reqLocation->alias : client._request._reqLocation->root;

	if (access(scriptPath.c_str(), F_OK) != 0) {
		Logger::error("CGI script not found: " + scriptPath);
		client._request.err = true;
		client._request.status = 404;
		client._request.findErrorPage(404, root, client._request._reqLocation->errPage);
		return (false);
	}

	struct stat	st;

	if (stat(scriptPath.c_str(), &st) != 0) {
		Logger::error("Cannot stat CGI script: " + scriptPath);
		client._request.err = true;
		client._request.status = 404;
		client._request.findErrorPage(404, root, client._request._reqLocation->errPage);
		return (false);
	}

	if (!S_ISREG(st.st_mode)) {
		Logger::error("CGI script is not a regular file: " + scriptPath);
		client._request.err = true;
		client._request.status = 404;
		client._request.findErrorPage(404, root, client._request._reqLocation->errPage);
		return (false);
	}

	if (access(scriptPath.c_str(), R_OK) != 0) { // R_OK -> readable file
		Logger::error("CGI script is not readable: " + scriptPath);
		client._request.err = true;
		client._request.status = 403;
		client._request.findErrorPage(403, root, client._request._reqLocation->errPage);
		return (false);
	}

	return (true);
}

std::string	CGIExecutor::getDirectoryFromPath(const std::string& path) {

	size_t	lastSlash = path.find_last_of('/');

	if (lastSlash == std::string::npos) {
		return (".");  // current directory
	}

	if (lastSlash == 0) {
		return ("/");  // root file
	}

	return (path.substr(0, lastSlash));
}
