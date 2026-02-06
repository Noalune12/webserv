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

std::vector<std::string>	CGIExecutor::buildEnvironmentStrings(const Connection& conn) {

	std::vector<std::string>	envStrings;

	envStrings.push_back(buildEnvVar("REQUEST_METHOD", conn._request._method));
	envStrings.push_back(buildEnvVar("QUERY_STRING", conn._request._queryString));
	envStrings.push_back(buildEnvVar("SCRIPT_FILENAME", conn._request._scriptPath));
	envStrings.push_back(buildEnvVar("SERVER_PROTOCOL", std::string("HTTP/" + conn._request._version)));
	envStrings.push_back(buildEnvVar("GATEWAY_INTERFACE", "CGI/1.1"));
	envStrings.push_back(buildEnvVar("REDIRECT_STATUS", "200"));
	envStrings.push_back(buildEnvVar("SCRIPT_NAME", conn._request._uri));

	// PATH_INFO (extra path after script name, if any)
	envStrings.push_back(buildEnvVar("PATH_INFO", ""));
	// temporary test for PATH_INFO, not implemented yet. I'll parse it myself
	envStrings.push_back(buildEnvVar("PATH_INFO2", conn._request._trailing));


	// Content headers
	std::map<std::string, std::string>::const_iterator	it;
	it = conn._request._headers.find("content-length");
	if (it != conn._request._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", it->second));
	} else {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", "0"));
	}

	it = conn._request._headers.find("content-type");
	if (it != conn._request._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_TYPE", it->second));
	}

	// Server information
	if (conn._request._reqLocation && conn._request._reqServer && !conn._request._reqServer->lis.empty() && !conn._request._reqServer->serverName.empty()) {
		std::stringstream ss;
		ss << conn._request._reqServer->lis[0].port;
		envStrings.push_back(buildEnvVar("SERVER_NAME", conn._request._reqServer->serverName[0]));
		envStrings.push_back(buildEnvVar("SERVER_PORT", ss.str()));
	}

	envStrings.push_back(buildEnvVar("REMOTE_ADDR", conn.getIP()));

	for (it = conn._request._headers.begin(); it != conn._request._headers.end(); ++it) {

		std::string	headerName = it->first;
		std::string	headerValue = it->second;

		std::string envName = "HTTP_";
		for (size_t i = 0; i < headerName.size(); ++i) {
			char c = headerName[i];
			if (c == '-') {
				envName += '_';
			} else {
				envName += std::toupper(static_cast<unsigned char>(c));
			}
		}

		// Skip Content-Length and Content-Type (already handled above)
		if (envName != "HTTP_CONTENT_LENGTH" && envName != "HTTP_CONTENT_TYPE") {
			envStrings.push_back(buildEnvVar(envName, headerValue));
		}
	}

	return (envStrings);
}

std::string	CGIExecutor::buildEnvVar(const std::string& name, const std::string& value) {
	return (name + "=" + value);
}
