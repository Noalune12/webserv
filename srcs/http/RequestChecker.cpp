#include "Request.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

void Request::findServer() {
	std::vector<server>::iterator itServer = _servers.begin();
	bool serverFound = false;
	for (; itServer != _servers.end(); itServer++) {
		std::vector<listenDirective>::iterator itListen = itServer->lis.begin();
		for (; itListen != itServer->lis.end(); itListen++) {
			if (itListen->port == _serverPort && (itListen->ip == _serverName || itListen->ip == "0.0.0.0")) {
				// what about servername and virtual hosting
				std::cout << "server found with " << itListen->port << ", " << itListen->ip << std::endl;
				_reqServer = &(*itServer);
				serverFound = true;
				return ;
			}
		}
	}
}

void Request::findLocation() {
	// uri check
    // /!\ check only 1st /.../
	std::vector<location>::iterator itLocation = _reqServer->loc.begin();
	for (; itLocation != _reqServer->loc.end(); itLocation++) {
		if (_uri == itLocation->path || (_uri + "/") == itLocation->path || _uri == itLocation->path + "/" ) {
			std::cout << "location found at " << itLocation->path << std::endl;
			_reqLocation = &(*itLocation);
			return ;
		}
	}
}

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
		_serverPort = 8080;
	} else {
        _serverName = it->second.substr(0, sep);
		std::string temp = it->second.substr(sep + 1, it->second.size());
        if (!isOnlyDigits(temp)) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error host port has not only digits" << std::endl;
            return false;
        }
		std::stringstream ss(temp);
		ss >> _serverPort; // what if overflow
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

    std::cout << "\nCHECK REQUEST CONTENT\n" << std::endl;
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
        findErrorPage(405, _reqLocation->root, _reqLocation->errPage);
	    std::cout << "error not allowed method" << std::endl;
		return ;
	}

    bodyChecker();
    // if method is POST but no body => 400

}

bool Request::bodyChecker() {
    // find transfer encoding
    std::map<std::string, std::string>::iterator it = _headers.find("transfer-encoding");
    if (it != _headers.end() && it->second != "chunked") {
        findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        std::cout << "error transfer encoding not well defined" << std::endl;
        return false;
    } else if (it != _headers.end() && it->second == "chunked") {
        std::cout << "PARSE CHUNKED BODY HERE" << std::endl;
        if (_body.size() >= 5 &&
                _body.compare(_body.size() - 5, 5, "0\r\n\r\n") == 0) {
                chunkRemaining = false;
                _body = _body.substr(0, _body.size() - 5);
                parseChunk();
        } else {
            chunkRemaining = true;
            _chunk = _body;
            _body.clear();
            _chunkState = GETTING_FIRST_SIZE;
        }

    } else if (it == _headers.end()) {
        if (_reqLocation->bodySize < _body.size()) {
            findErrorPage(413, _reqLocation->root, _reqLocation->errPage);
            std::cout << "error body is higher that client max body size" << std::endl;
            return false;   
        }

        // find content length
        
        it = _headers.find("content-length");
        if (it == _headers.end() && !_body.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            std::cout << "error no content length but existing body" << std::endl;
            return false;
        } else if (it != _headers.end()) {
            std::cout << "CONTENT LENGTH = " << it->second << std::endl;
            std::stringstream ss;
            ss << _body.size();
            std::string bodySize = ss.str();
            std::cout << "BODY SIZE = " << bodySize << std::endl;
            if (it->second != bodySize) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                std::cout << "error  content length is not equal to existing body size" << std::endl;
                return false;
            }
        }
    }
    return true;
}