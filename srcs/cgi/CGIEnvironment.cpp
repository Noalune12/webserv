#include <cstring>
#include "errno.h"
#include <signal.h>
#include <sstream>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "CGIExecutor.hpp"
#include "Logger.hpp"

std::vector<std::string>	CGIExecutor::buildEnvironmentStrings(const Connection& client) {

	std::vector<std::string>	envStrings;

	std::string	full_path = client._request._uri;
	size_t		queryPos = full_path.find('?');
	if (queryPos != std::string::npos) {
		full_path = full_path.substr(0, queryPos);
	}

	std::string	script_name = client._request._scriptPath;
	size_t		script_query = script_name.find('?');
	if (script_query != std::string::npos) {
		script_name = script_name.substr(0, script_query);
	}

	std::string	path_info = "";
	size_t		script_pos = full_path.find(script_name);
	if (script_pos != std::string::npos && static_cast<size_t>(script_pos + script_name.length()) < full_path.length()) {
		path_info = full_path.substr(script_pos + script_name.length());
	}

	std::string qstring = client._request._queryString;
	size_t	qstring_pos = client._request._queryString.find('?');
	if (qstring_pos != std::string::npos) {
		qstring = qstring.substr(qstring_pos + 1);
	}

	envStrings.push_back(buildEnvVar("REQUEST_METHOD", client._request._method));
	envStrings.push_back(buildEnvVar("QUERY_STRING", qstring));
	envStrings.push_back(buildEnvVar("SCRIPT_FILENAME", client._request._scriptPath));
	envStrings.push_back(buildEnvVar("SCRIPT_NAME", script_name));
	envStrings.push_back(buildEnvVar("PATH_INFO", path_info));
	envStrings.push_back(buildEnvVar("SERVER_PROTOCOL", std::string("HTTP/") + client._request._version));
	envStrings.push_back(buildEnvVar("GATEWAY_INTERFACE", "CGI/1.1"));
	envStrings.push_back(buildEnvVar("REDIRECT_STATUS", "200"));
	envStrings.push_back(buildEnvVar("SERVER_SOFTWARE", "webserv/1.0"));

	if (client._request._reqLocation && client._request._reqServer &&
		!client._request._reqServer->lis.empty() && !client._request._reqServer->serverName.empty()) {
		std::stringstream	ss;
		ss << client._request._reqServer->lis[0].port;
		envStrings.push_back(buildEnvVar("SERVER_NAME", client._request._reqServer->serverName[0]));
		envStrings.push_back(buildEnvVar("SERVER_PORT", ss.str()));
	}

	envStrings.push_back(buildEnvVar("REMOTE_ADDR", client.getIP()));

	std::map<std::string, std::string>::const_iterator	it;
	it = client._request._headers.find("content-length");
	if (it != client._request._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", it->second));
	} else {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", "0"));
	}
	it = client._request._headers.find("content-type");
	if (it != client._request._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_TYPE", it->second));
	}

	for (it = client._request._headers.begin(); it != client._request._headers.end(); ++it) {
		std::string	headerName = it->first;
		std::string	headerValue = it->second;
		std::string	envName = "HTTP_";
		for (size_t i = 0; i < headerName.size(); ++i) {
			char c = headerName[i];
			if (c == '-') {
				envName += '_';
			} else {
				envName += std::toupper(static_cast<unsigned char>(c));
			}
		}
		if (envName != "HTTP_CONTENT_LENGTH" && envName != "HTTP_CONTENT_TYPE") {
			envStrings.push_back(buildEnvVar(envName, headerValue));
		}
	}

	return (envStrings);
}

std::string	CGIExecutor::buildEnvVar(const std::string& name, const std::string& value) {
	return (name + "=" + value);
}
