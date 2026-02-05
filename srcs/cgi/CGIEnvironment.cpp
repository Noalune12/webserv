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

std::vector<std::string>	CGIExecutor::buildEnvironmentStrings(const Request& req) {

	std::vector<std::string>	envStrings;

	envStrings.push_back(buildEnvVar("REQUEST_METHOD", req._method));
	envStrings.push_back(buildEnvVar("QUERY_STRING", req._queryString));
	envStrings.push_back(buildEnvVar("SCRIPT_FILENAME", req._scriptPath));
	envStrings.push_back(buildEnvVar("SERVER_PROTOCOL", "HTTP/1.1"));
	envStrings.push_back(buildEnvVar("GATEWAY_INTERFACE", "CGI/1.1"));
	envStrings.push_back(buildEnvVar("REDIRECT_STATUS", "200"));

	std::map<std::string, std::string>::const_iterator it;
	it = req._headers.find("Content-Length");
	if (it != req._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", it->second));
	} else {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", "0"));
	}

	it = req._headers.find("Content-Type");
	if (it != req._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_TYPE", it->second));
	}

	if (req._reqLocation) {
		std::stringstream ss;
		ss << req._reqServer->lis[0].port;
		envStrings.push_back(buildEnvVar("SERVER_NAME", req._reqServer->serverName[0]));
		envStrings.push_back(buildEnvVar("SERVER_PORT", ss.str()));

		std::cerr <<  "HERE: " << req._reqServer->serverName[0] << "\nport: " << ss.str() << std::endl;
	}

	return (envStrings);
}

std::string	CGIExecutor::buildEnvVar(const std::string& name, const std::string& value) {
	return (name + "=" + value);
}
