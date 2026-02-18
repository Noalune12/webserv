#include "Request.hpp"
#include "Logger.hpp"

#include <iostream>
#include <sstream>
#include <sys/stat.h>

void Request::findServer() {

	std::vector<server>::iterator itServer = _servers.begin();
    std::vector<size_t> possibleServerIndices;
	for (; itServer != _servers.end(); itServer++) {
		std::vector<listenDirective>::iterator itListen = itServer->lis.begin();
        if (itServer->isRunning == false)
            continue ;
		for (; itListen != itServer->lis.end(); itListen++) {
			if (itListen->port == _serverPort && (itListen->ip == _serverName || itListen->ip == "0.0.0.0" || itListen->ip == _serverIp)) {
                possibleServerIndices.push_back(std::distance(_servers.begin(), itServer));
                break ;
			}
		}
	}
    for (size_t i = 0; i < possibleServerIndices.size(); i++) {
        size_t serverIndex = possibleServerIndices[i];
        server& candidateServer = _servers[serverIndex];

        std::vector<std::string>::iterator itServerName = candidateServer.serverName.begin();
        for (; itServerName != candidateServer.serverName.end(); itServerName++) {
            if (*itServerName == _serverName) {
                reqServer = &_servers[serverIndex];
                break;
            }
        }
        if (reqServer != NULL)
            break;
    }

    if (reqServer == NULL && possibleServerIndices.size() > 0) { 
        size_t serverIndex = possibleServerIndices[0];
        reqServer = &_servers[serverIndex];
    }
}


void Request::findLocation() {
    size_t  query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        queryString = uri.substr(query_pos + 1);
        uri = uri.substr(0, query_pos);
    }

    while (!uri.empty()) {
        std::vector<location>::iterator itLocation = reqServer->loc.begin();
        for (; itLocation != reqServer->loc.end(); itLocation++) {
            if (uri == itLocation->path || uri + "/" == itLocation->path || uri == itLocation->path + "/") {
                reqLocation = &(*itLocation);
                return ;
            }
        }
        if (uri == "/")
            break;
        size_t index = uri.rfind('/');
        if (index == uri.size() - 1) {
            uri = uri.substr(0, index);
            trailing = "/" + trailing;
            index = uri.rfind('/');
        }
        trailing = uri.substr(index + 1, uri.size()) + trailing;
        uri = uri.substr(0, index + 1);
    }
}

#include <limits.h>

bool Request::hostChecker() {
    // Host check
	std::map<std::string, std::string>::iterator it = headers.find("host");
	if (it == headers.end()) {
		findErrorPage(400, "/", _globalDir.errPage);
	    Logger::warn("Headers: missing Host");
		return false;
	}
	size_t sep = it->second.find(":");
	if (sep == 0 || sep == it->second.size() - 1) {
		findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Headers: Host not well defined");
		return false;
	}
	if (sep == std::string::npos) {
		_serverName = it->second;
	} else {
        double port;
        _serverName = it->second.substr(0, sep);
		std::string temp = it->second.substr(sep + 1, it->second.size());
        if (!isOnlyDigits(temp)) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers: Host Port not well defined");
            return false;
        }
		std::stringstream ss(temp);
		ss >> port;
        if (ss.fail() || !ss.eof()) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers: Host Port not well defined");
            return false;
        }

        if (port != _serverPort) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers: Host Port not well defined");
            return false;
        }
	}
	if (_serverName == "localhost") {
		_serverName = "127.0.0.1";
	}

    findServer();
	if (reqServer == NULL) {
		findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Headers: no compatible server found");
		return false;
	}

    findLocation();
	if (reqLocation == NULL) {
        findErrorPage(404, reqServer->root, reqServer->errPage);
	    Logger::warn("Headers: no location found");
		return false;
	}

    return true;
}

void Request::checkRequestContent() {

    std::map<std::string, std::string>::iterator it = headers.find("connection");
    if (it == headers.end() || it->second == "close")
        keepAlive = false;
    else if (it->second == "keep-alive")
        keepAlive = true;
    else {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Headers: Connection not well defined");
		return ;
    }

    if (!hostChecker()) {
        return ;
    }

	// method check
	if ((method == "GET" && reqLocation->methods.get == false)
			|| (method == "POST" && reqLocation->methods.post == false)
            || (method == "DELETE" && reqLocation->methods.del == false)
            || method == "HEAD" || method == "OPTIONS"
            || method == "TRACE" || method == "PUT"
            || method == "PATCH" || method == "CONNECT") {
        if (!reqLocation->root.empty()) {
            findErrorPage(405, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(405, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Headers: Method is not supported");
		return ;
	}

    bodyChecker();

    if (!reqLocation->returnPath.empty()) {
        if (!trailing.empty())
            return;
        status = reqLocation->returnStatus;
        returnPath = reqLocation->returnPath;
        returnDirective = true;
    }

    if (!reqLocation->cgiExt.empty() && !reqLocation->cgiPath.empty()) {
        if (trailing.empty())
            return;
        if (!reqLocation->root.empty())
            scriptPath = getPath(reqLocation->root + uri, trailing);
        else
            scriptPath = getPath(reqLocation->alias, trailing);
        struct stat buf;

        if (stat(scriptPath.c_str(), &buf) == 0) {
            Logger::debug("CGI: File Found");
            isCgi = true;
        }
        else {
            if (!reqLocation->root.empty()) {
                findErrorPage(404, reqLocation->root, reqLocation->errPage);
            }
            else {
                findErrorPage(404, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("CGI: path not found");
        }
    }

}


