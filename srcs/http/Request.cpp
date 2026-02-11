#include "Request.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include "colors.hpp"

Request::Request(): err(false), status(0), chunkRemaining(false), _keepAlive(false) {}

Request::Request(std::vector<server> servers, globalDir globalDir) : _servers(servers), _globalDir(globalDir), err(false), status(0), chunkRemaining(false), _isChunked(false), _keepAlive(false), _cgi(false), _return(false), _indexFound(false), _isMultipart(false), _multipartRemaining(false), _remainingBody(false) {}


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
    _trailing.clear();
    _cgi = false;
    _return = false;
   _scriptPath.clear();
   _queryString.clear();
   _postExt.clear();
   _postFilename.clear();
   _indexFound = false;
   _getPath.clear();
   _isMultipart = false;
   _multipartBoundary.clear();
   _multipartContent.clear();
   _fullBody.clear();
   _multipartRemaining = false;
    // what about err and keep alive
    _method.clear();
    _uri.clear();
    _version.clear();
    err = false;
    _keepAlive = false;
    chunkRemaining = false;
    status = 200;
    _isChunked = false;
    _uplaodFiles.clear();
    _failedUpload = 0;
    _totalUpload = 0;
    _remainingBody = false;
    _multiTemp.headers.clear();
    _multiTemp.body.clear();
    _multiTemp.filename.clear();
    _multiTemp.name.clear();
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

std::string Request::trimFirstCRLF(const std::string& s)
{
    std::string::size_type start = 0;
    std::string::size_type end = s.size();

    while (start < end && (s[start] == '\r' || s[start] == '\n'))
        ++start;

    return s.substr(start, end - start);
}


void Request::findErrorPage(int code, std::string root, std::map<int, std::string> errPage) {
    err = true;
    status = code;
    std::map<int, std::string>::iterator itErr = errPage.find(code);
    if (itErr == errPage.end())
        return;
    std::string path;
    if (root[root.size() - 1] == '/' && itErr->second[0] == '/')
        path = root.substr(0, root.size() - 1) + itErr->second;
    else if (root[root.size() - 1] != '/' && itErr->second[0] != '/')
        path = root + "/" + itErr->second;
    else
        path = root + itErr->second;

    if (path[0] == '/') // what if many /
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
        methodGetHandler();
	} else if (_method == "DELETE") {
        methodDeleteHandler();
    } else if (_method == "POST") {
        methodPostHandler();
    }
}



void Request::setServerInfo(const int& port, const std::string& ip) {
    _serverIp = ip;
    _serverPort = port;
}


bool Request::isCRLF(std::string request) {
    std::cout << "IS CRLF BUFFER = " << request << std::endl;
    if (request.find("\r\n\r\n") != std::string::npos)
        return true;
    return false;
}