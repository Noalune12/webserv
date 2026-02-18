#include <sstream>

#include "CGIExecutor.hpp"

std::vector<std::string>	CGIExecutor::buildEnvironmentStrings(const Connection& client) {

	std::vector<std::string>	envStrings;

	std::string	full_path = client.request.uri;
	size_t		queryPos = full_path.find('?');
	if (queryPos != std::string::npos) {
		full_path = full_path.substr(0, queryPos);
	}

	std::string	script_name = client.request.scriptPath;
	size_t		script_query = script_name.find('?');
	if (script_query != std::string::npos) {
		script_name = script_name.substr(0, script_query);
	}

	std::string	path_info = "";
	size_t		script_pos = full_path.find(script_name);
	if (script_pos != std::string::npos && static_cast<size_t>(script_pos + script_name.length()) < full_path.length()) {
		path_info = full_path.substr(script_pos + script_name.length());
	}

	std::string qstring = client.request.queryString;
	size_t	qstring_pos = client.request.queryString.find('?');
	if (qstring_pos != std::string::npos) {
		qstring = qstring.substr(qstring_pos + 1);
	}

	envStrings.push_back(buildEnvVar("REQUEST_METHOD", client.request.method));
	envStrings.push_back(buildEnvVar("QUERY_STRING", qstring));
	envStrings.push_back(buildEnvVar("SCRIPT_FILENAME", client.request.scriptPath));
	envStrings.push_back(buildEnvVar("SCRIPT_NAME", client.request.uri + script_name));
	envStrings.push_back(buildEnvVar("PATH_INFO", path_info));
	envStrings.push_back(buildEnvVar("SERVER_PROTOCOL", std::string("HTTP/") + client.request.version));
	envStrings.push_back(buildEnvVar("GATEWAY_INTERFACE", "CGI/1.1"));
	envStrings.push_back(buildEnvVar("REDIRECT_STATUS", "200"));
	envStrings.push_back(buildEnvVar("SERVER_SOFTWARE", "webserv/1.0"));

	if (client.request.reqLocation && client.request.reqServer &&
		!client.request.reqServer->lis.empty() && !client.request.reqServer->serverName.empty()) {
		std::stringstream	ss;
		ss << client.request.reqServer->lis[0].port;
		envStrings.push_back(buildEnvVar("SERVER_NAME", client.request.reqServer->serverName[0]));
		envStrings.push_back(buildEnvVar("SERVER_PORT", ss.str()));
	}

	envStrings.push_back(buildEnvVar("REMOTE_ADDR", client.getIP()));

	std::map<std::string, std::string>::const_iterator	it;
	it = client.request.headers.find("content-length");
	if (it != client.request.headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", it->second));
	} else {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", "0"));
	}
	it = client.request.headers.find("content-type");
	if (it != client.request.headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_TYPE", it->second));
	}

	for (it = client.request.headers.begin(); it != client.request.headers.end(); ++it) {
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
