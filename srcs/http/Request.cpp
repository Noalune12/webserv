#include "Request.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

Request::Request(): err(false), status(0), chunkRemaining(false), _keepAlive(false) {}

Request::Request(std::vector<server> servers, globalDir globalDir) : _servers(servers), _globalDir(globalDir), err(false), status(0), chunkRemaining(false), _keepAlive(false) {}


Request::~Request() {}

void Request::printWithoutR(std::string what, std::string line) const {
    std::string l;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] != '\r')
            l.push_back(line[i]);
    }
    std::cout << what <<" = \'" << l << "\' -" << std::endl;
}

void Request::clearPreviousRequest() {
    htmlPage.clear();
    _headersStr.clear();
    _requestLine.clear();
    _body.clear();
    _headers.clear();
    _chunk.clear();
    _reqServer = NULL;
    _reqLocation = NULL;
    // what about err and keep alive
    // _trailing.clear();
}

bool Request::hasWS(const std::string& line) const {
    for (size_t i = 0; i < line.length(); i++) {
        if (isspace(line[i]))
            return true;
    }
    return false;
}

bool Request::isOnlyDigits(const std::string& line) const {
	size_t count = 0;

	for (size_t i = 0; i < line.length(); ++i) {
        if (isdigit(line[i]))
            count++;
    }
	if (line.length() == count)
		return true;
	return false;
}

std::string Request::trimOws(const std::string& s)
{
    std::string::size_type start = 0;
    std::string::size_type end = s.size();

    // trim leading spaces/tabs, what about other whitespaces
    while (start < end && (s[start] == ' ' || s[start] == '\t'))
        ++start;

    // trim trailing spaces/tabs, what about other whitespaces
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
        --end;

    return s.substr(start, end - start);
}

void Request::findErrorPage(int code, std::string root, std::map<int, std::string> errPage) {
    err = true;
    status = code;
    // root = root.substr(1, root.size()); // what if no root ? if starts with \ need to check ?
    std::map<int, std::string>::iterator itErr = errPage.find(code);
    if (itErr == errPage.end())
        return;
    std::string path = root + itErr->second;
    if (path[0] == '/')
        path = path.substr(1, path.size());
    std::cout << "PATH = " << path << std::endl;
			std::ifstream file(path.c_str());
			if (!file) {
				return;
			} else {
				std::cout << "File found" << std::endl;
				std::stringstream buffer;
				buffer << file.rdbuf();
				htmlPage = buffer.str();
			}
}

void Request::methodHandler() {
	// get info
	if (_method == "GET") {
		std::vector<std::string>::iterator itIndex = _reqLocation->index.begin();

        // IF ROOT
        if (!_reqLocation->root.empty()) {
            std::string root = _reqLocation->root; // what if no root ? if starts with / need to check ?
            for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                std::string path;
                if (_uri[_uri.size() - 1] == '/')
                    path = root + _uri + *itIndex; // what if directory does not exist ...
                else
                    path = root + _uri + "/" + *itIndex;
                if (path[0] == '/')
                    path = path.substr(1, path.size());
                std::cout << "PATH = " << path << std::endl;
                // check access
                std::ifstream file(path.c_str());
                if (!file) {
                    continue;
                } else {
                    std::cout << "File found" << std::endl;
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    htmlPage = buffer.str();
                    // htmlPage.assign(
                    // 	(std::istreambuf_iterator<char>(file)),
                    // 	std::istreambuf_iterator<char>());
                    err = false;
                    status = 200;
                    return ;
                }
            }
        } else {
            std::string alias = _reqLocation->alias;
            for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                std::string path;
                if (_uri[_uri.size() - 1] == '/')
                    path = alias + *itIndex; 
                else
                    path = alias + "/" + *itIndex;
                if (path[0] == '/')
                    path = path.substr(1, path.size());
                std::cout << "PATH = " << path << std::endl;
                // check access
                std::ifstream file(path.c_str());
                if (!file) {
                    continue;
                } else {
                    std::cout << "File found" << std::endl;
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    htmlPage = buffer.str();
                    // htmlPage.assign(
                    // 	(std::istreambuf_iterator<char>(file)),
                    // 	std::istreambuf_iterator<char>());
                    err = false;
                    status = 200;
                    return ;
                }
            }
        }

        // IF ALIAS
	
		if (htmlPage.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
			std::cout << "error no index found" << std::endl;
			return ;
		}
	}
}