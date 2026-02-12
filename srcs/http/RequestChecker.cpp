#include "Request.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>

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
                _reqServer = &_servers[serverIndex];
                break;
            }
        }
        if (_reqServer != NULL)
            break;
    }

    if (_reqServer == NULL && possibleServerIndices.size() > 0) { 
        size_t serverIndex = possibleServerIndices[0];
        _reqServer = &_servers[serverIndex];
    }
}


void Request::findLocation() {
    size_t  query_pos = _uri.find('?');
    if (query_pos != std::string::npos) {
        _queryString = _uri.substr(query_pos + 1);
        _uri = _uri.substr(0, query_pos);
    }

    while (!_uri.empty()) {
        std::vector<location>::iterator itLocation = _reqServer->loc.begin();
        for (; itLocation != _reqServer->loc.end(); itLocation++) {
            if (_uri == itLocation->path || _uri + "/" == itLocation->path || _uri == itLocation->path + "/") {
                _reqLocation = &(*itLocation);
                return ;
            }
        }
        if (_uri == "/")
            break;
        size_t index = _uri.rfind('/');
        if (index == _uri.size() - 1) {
            _uri = _uri.substr(0, index);
            _trailing = "/" + _trailing;
            index = _uri.rfind('/');
        }
        _trailing = _uri.substr(index + 1, _uri.size()) + _trailing;
        _uri = _uri.substr(0, index + 1);
    }
}

#include <limits.h>

bool Request::hostChecker() {
    // Host check
	std::map<std::string, std::string>::iterator it = _headers.find("host");
	if (it == _headers.end()) {
		findErrorPage(400, "/", _globalDir.errPage);
	    std::cout << "error no host in headers " << std::endl;
		return false;
	}
	size_t sep = it->second.find(":");
	if (sep == 0 || sep == it->second.size() - 1) {
		findErrorPage(400, "/", _globalDir.errPage);
	    std::cout << "error host has : at the start or the end" << std::endl;
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
            std::cout << "error host port has not only digits" << std::endl;
            return false;
        }
		std::stringstream ss(temp);
		ss >> port;
        if (ss.fail() || !ss.eof()) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error while converting port" << std::endl;
            return false;
        }

        if (port != _serverPort) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error host port is not the same as server" << std::endl;
            return false;
        }
	}
	if (_serverName == "localhost") {
		_serverName = "127.0.0.1";
	}

    findServer();
	if (_reqServer == NULL) {
		findErrorPage(400, "/", _globalDir.errPage);
	    std::cout << "error no compatible server found" << std::endl;
		return false;
	}

    findLocation();
	if (_reqLocation == NULL) {
        findErrorPage(404, _reqServer->root, _reqServer->errPage);
	    std::cout << "error no location found" << std::endl;
		return false;
	}

    return true;
}

void Request::checkRequestContent() {

    std::map<std::string, std::string>::iterator it = _headers.find("connection");
    if (it == _headers.end() || it->second == "close")
        _keepAlive = false;
    else if (it->second == "keep-alive")
        _keepAlive = true;
    else {
        findErrorPage(400, "/", _globalDir.errPage);
	    std::cout << "error in headers: connection not well formated " << std::endl;
		return ;
    }

    if (!hostChecker()) {
        return ;
    }

	// method check
	if ((_method == "GET" && _reqLocation->methods.get == false)
			|| (_method == "POST" && _reqLocation->methods.post == false)
            || (_method == "DELETE" && _reqLocation->methods.del == false)
            || _method == "HEAD" || _method == "OPTIONS"
            || _method == "TRACE" || _method == "PUT"
            || _method == "PATCH" || _method == "CONNECT") {
        if (!_reqLocation->root.empty()) {
            findErrorPage(405, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(405, _reqLocation->alias, _reqLocation->errPage);
        }
	    std::cout << "error not allowed method" << std::endl;
		return ;
	}

    bodyChecker();

    if (!_reqLocation->returnPath.empty()) {
        if (!_trailing.empty())
            return;
        status = _reqLocation->returnStatus;
        _returnPath = _reqLocation->returnPath;
        _return = true;
    }

    if (!_reqLocation->cgiExt.empty() && !_reqLocation->cgiPath.empty()) {
        if (_trailing.empty() /*|| _method != "POST" */) // IMPORTANT -> discussion about that method check
            return;
        // size_t index = _trailing.find('?'); //what if the folder has a ?
        // if (index != std::string::npos) {
        //     _queryString = _trailing.substr(index);
        //     _trailing = _trailing.substr(0, index);
        // }
        if (!_reqLocation->root.empty())
            _scriptPath = getPath(_reqLocation->root + _uri, _trailing);
        else
            _scriptPath = getPath(_reqLocation->alias, _trailing);
        struct stat buf;

        if (stat(_scriptPath.c_str(), &buf) == 0) {
            std::cout << "FILE FOUND FOR CGI" << std::endl;
            _cgi = true;
        }
        else {
            if (!_reqLocation->root.empty()) {
                findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with CGI file or folder note found" << std::endl;
        }
    }

}


