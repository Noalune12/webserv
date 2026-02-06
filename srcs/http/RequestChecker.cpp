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
        // std::cout << "Is server running ? " << itServer->isRunning << std::endl;
        if (itServer->isRunning == false)
            continue ;
		for (; itListen != itServer->lis.end(); itListen++) {
            // std::cout << "POST it = " << itListen->port << "ServerPort = "<< _serverPort << "IP it = "<< itListen->ip << "ServerName = "<< _serverName << std::endl;
			if (itListen->port == _serverPort && (itListen->ip == _serverName || itListen->ip == "0.0.0.0" || itListen->ip == _serverIp)) {
				std::cout << "possible server found with " << itListen->port << ", " << itListen->ip << std::endl;
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
                std::cout << "server found with " << _serverName << std::endl;
                _reqServer = &_servers[serverIndex]; 
                break;
            }
        }    
        if (_reqServer != NULL)
            break;
    }
    // _reqServer = &(*itServer);
    // std::cout << "SERVER FOUND = " << serverFound << std::endl;
    if (_reqServer == NULL && possibleServerIndices.size() > 0) { // how to decide which server to chose, by default I set the 1st one
        size_t serverIndex = possibleServerIndices[0];
        _reqServer = &_servers[serverIndex]; 
        std::cout << "default server is set" << std::endl;
    }
    // _reqServer = NULL;
}


void Request::findLocation() {
	// uri check
    // /!\ check only 1st /.../
    // size_t index = _uri.rfind('/');
    // if (index != _uri.size() - 1) {
    //     _trailing = _uri.substr(index + 1, _uri.size());
    //     _uri = _uri.substr(0, index + 1);
    // }
    std::cout << "before loop URI = \'" << _uri << "\'" << " -------- TRAILING = \'" << _trailing << "\'" << std::endl;
    while (!_uri.empty()) {
        std::vector<location>::iterator itLocation = _reqServer->loc.begin();
        for (; itLocation != _reqServer->loc.end(); itLocation++) {
            if (_uri == itLocation->path || _uri + "/" == itLocation->path || _uri == itLocation->path + "/") {
                std::cout << "location found at " << itLocation->path << std::endl;
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
        std::cout << "in loop URI = \'" << _uri << "\'" << " -------- TRAILING = \'" << _trailing << "\'" << std::endl;
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
		_serverName = it->second; // can be server name or ip
	} else {
        int port;
        _serverName = it->second.substr(0, sep);
		std::string temp = it->second.substr(sep + 1, it->second.size());
        if (!isOnlyDigits(temp)) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error host port has not only digits" << std::endl;
            return false;
        }
		std::stringstream ss(temp);
		ss >> port; // what if overflow
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
        htmlPage = "this will be a return";
        // std::map<int, std::string>::iterator it = _reqLocation->ret.begin();
        status = _reqLocation->returnStatus;
        _returnPath = _reqLocation->returnPath;
        std::cout << "RETUEN STATUS " << status << "with path " << _returnPath << std::endl;
        _return = true;
    }

    if (!_reqLocation->cgiExt.empty() && !_reqLocation->cgiPath.empty()) {
        if (_trailing.empty() || _method != "POST")
            return;
        size_t index = _trailing.find('?'); //what if the folder has a ?
        if (index != std::string::npos) {
            _queryString = _trailing.substr(index);
            _trailing = _trailing.substr(0, index);
        }
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
    // if method is POST but no body => 400

}

bool Request::bodyChecker() {
    // find transfer encoding
    std::map<std::string, std::string>::iterator it = _headers.find("transfer-encoding");
    if (it != _headers.end() && it->second != "chunked") {
        if (!_reqLocation->root.empty())
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error transfer encoding not well defined" << std::endl;
        return false;
    } else if (it != _headers.end() && it->second == "chunked") {
        std::cout << "PARSE CHUNKED BODY HERE" << std::endl;
        // if (_body.size() >= 5 &&
        //         _body.compare(_body.size() - 5, 5, "0\r\n\r\n") == 0) {
        //         chunkRemaining = false;
        //         _body = _body.substr(0, _body.size() - 5);
        //         _chunkState = GETTING_FIRST_SIZE;
        //         parseChunk();
        // } else {
            chunkRemaining = true;
            _chunk = _body;
            _body.clear();
            _chunkState = GETTING_FIRST_SIZE;
            parseChunk();
        // }

    } else if (it == _headers.end()) {

        // check if multipart
        it = _headers.find("content-type");
        if (it != _headers.end()) {
            checkMultipart(it->second);
            if (_isMultipart) {
                std::cout << "\nMULTIPART PARSING" << std::endl;
                _multipartState = GETTING_FIRST_BOUNDARY;
                _fullBody = _body;
                if (!parseMultipart())
                    return false;
                return true;
            } else {
                std::transform(it->second.begin(), it->second.end(), it->second.begin(), ::tolower);
            }
        }


        if (_reqLocation->bodySize < _body.size()) {
            if (!_reqLocation->root.empty())
                findErrorPage(413, _reqLocation->root, _reqLocation->errPage);
            else
                findErrorPage(413, _reqLocation->alias, _reqLocation->errPage);
            std::cout << "error body is higher that client max body size" << std::endl;
            return false;   
        }

        // find content length
        
        it = _headers.find("content-length");
        if (it == _headers.end() && !_body.empty()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);            
            }
            std::cout << "error no content length but existing body" << std::endl;
            return false;
        } else if (it != _headers.end()) {
            std::cout << "CONTENT LENGTH = " << it->second << std::endl;
            std::stringstream ss;
            ss << _body.size();
            std::string bodySize = ss.str();
            std::cout << "BODY SIZE = " << bodySize << std::endl;
            if (it->second != bodySize) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);                
                }
                std::cout << "error  content length is not equal to existing body size" << std::endl;
                return false;
            }
        }
    }
    return true;
}

void Request::checkMultipart(std::string content) {
    std::cout << "\nCHECK MULTIPART : " << content << std::endl;
    size_t index = content.find(";");
    std::string type, boundary;
    if (index == std::string::npos)
        return ;
    else { 
        type = content.substr(0, index);
        boundary = content.substr(index + 1);
        boundary = trimOws(boundary);
        std::cout << type << " with boundary \'" << boundary << "\'" << std::endl;
        index = boundary.find("=");
        if (index == 8) {
            std::string b = boundary.substr(0, 9);
            std::transform(b.begin(), b.end(), b.begin(), ::tolower);
            std::cout << "B = " << b << std::endl;
            if (b != "boundary=") {
                // eroor ?
                return;
            }
            boundary = boundary.substr(9);
            std::cout << "BOUNDARY IS = " << boundary << std::endl;
            if (boundary.empty() || boundary.size() > 70) {
                // error ?
                return;
            }
            if (boundary[0] == '\"' && boundary[boundary.size() - 1] == '\"') {
                boundary = boundary.substr(1, boundary.size() - 1); // to test
            }
            if (!isMultipartCarac(boundary)) {
                //error ?
                return ;
            } else {
                _multipartBoundary = boundary;
            }
        } else {
            return ;
        }
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        if (type == "multipart/form-data")
            _isMultipart = true;
    }
}

bool Request::isMultipartCarac(std::string &boundary) {
    if (boundary.empty()) {
        return false;
    }

    for (std::string::size_type i = 0; i < boundary.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(boundary[i]);

        if (
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.'
        ) {
            continue;
        }

        return false;
    }

    return true;
}

bool Request::parseMultipart() {

    // check if full body > client max body size + if content len exist compare (if not chunked)

    if (_multipartState == GETTING_FIRST_BOUNDARY) {
        size_t index = _body.find("--" + _multipartBoundary + "\r\n");
        if (index != 0 && _body.size() > _multipartBoundary.size() + 4 ) {
            // error
            std::cout << "multipart: no first boundary" << std::endl; // to test
            return false;
        } else {
            _body = _body.substr(index + _multipartBoundary.size() + 4);
            std::cout << "body is after 1st boundary = " << _body << std::endl;
            _multipartState = IS_HEADERS;
        }
    }

    Multipart multi;
    while (_multipartState != IS_MULTI_END) {
        if (_multipartState == IS_HEADERS) {
            // case sensitive, only Content-Disposition and Content-Type -> 400 invalid multipart header or ignore
            size_t index = _body.find("\r\n");
            if (index == std::string::npos) {
                std::cout << "waiting for headers" << std::endl;
                _multipartRemaining = true;
                return true;
            } else if (index == 0) {
                std::cout << "end of headers" << std::endl;

                // check if Content-Disposition is here and if it is well formated + si content-type correspond a l'extension

                std::cout << "\nMULTIPART HEADER MAP\n" << std::endl;
                std::map<std::string, std::string>::iterator it = multi.headers.begin();
                for (; it != multi.headers.end(); it++) {
                    std::cout << "[" << it->first << ", " << it->second << "]" << std::endl;
                }
                
                _multipartState = IS_BODY;
            } else {
                std::string header = _body.substr(0, index);
                _body = _body.substr(index + 2);
                std::cout << "Getting header = " << header << "and remaining body is " << _body <<std::endl;

                size_t index = header.find(":");
                if (index == 0 || index == std::string::npos) {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
                    std::cout << "error with multipart headers no : or no header name" << std::endl;
                    return false;
                }
                std::string name = header.substr(0, index);
                std::cout << "HEADER NAME " << name << std::endl;
                if (hasWS(name)) {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
                    std::cout << "error with multiparts headers name has WS" << std::endl;
                    return false;
                }
                if (name != "Content-Disposition" && name != "Content-Type") {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
                    std::cout << "error with multiparts headers invalid" << std::endl;
                    return false;
                }
                
                std::string content = header.substr(index + 1);
                
                if (!content.empty() && (content[0] != ' ' && content[0] != '\t')) {
                    std::cout << "CONTENT = \'" << content << "\'" << std::endl;
                    findErrorPage(400, "/", _globalDir.errPage);
                    std::cout << "error with headers : is not followed by space" << std::endl;
                    return false;
                }
                content = trimOws(content);
                std::cout << "HEADER CONTENT " << content << std::endl;
                if ((name == "Content-Disposition" && multi.headers.find("Content-Disposition") != multi.headers.end()) 
                        || (name == "Content-Type" && multi.headers.find("Content-Type") != multi.headers.end())) {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
                    std::cout << "error with duplicate multipart headers : " << name << std::endl;
                    return false;  
                }
                if (((name == "Content-Disposition") && content.empty())
                        || ((name == "Content-Type") && content.empty())) {
                    findErrorPage(400, "/", _globalDir.errPage);
                    std::cout << "error multipart header : " << name << " cannot have an empty content" << std::endl;
                    return false;
                }

                multi.headers[name] = content;

            }
        }
        if (_multipartState == IS_BODY) {

            break ;
        }
        // size_t index = _body.find("--" + _multipartBoundary + "--");
        // if (index != std::string::npos) { // index == 0 and size ok
        //     std::cout << "end found at index " << index << std::endl;
        //     _multipartState = IS_MULTI_END;
        //     _multipartRemaining = false;
        // }
    }
    return true;
}
