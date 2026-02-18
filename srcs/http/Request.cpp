#include "Request.hpp"
#include "Logger.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

Request::Request(): _chunkSize(-1), _chunkState(GETTING_FIRST_SIZE), _multipartState(GETTING_FIRST_BOUNDARY), \
    _failedUpload(0), _totalUpload(0), _indexFound(false), err(false), status(0), keepAlive(false), \
    reqServer(NULL), reqLocation(NULL), isCgi(false), returnDirective(false), chunkRemaining(false), isChunked(false), \
    isMultipart(false), multipartRemaining(false), remainingBody(false) {}

Request::Request(std::vector<server> servers, globalDir globalDir) : _servers(servers), _globalDir(globalDir), \
    _chunkSize(-1), _chunkState(GETTING_FIRST_SIZE), _multipartState(GETTING_FIRST_BOUNDARY), \
    _failedUpload(0), _totalUpload(0), _indexFound(false), err(false), status(0), keepAlive(false), \
    reqServer(NULL), reqLocation(NULL), isCgi(false), returnDirective(false), chunkRemaining(false), isChunked(false), \
    isMultipart(false), multipartRemaining(false), remainingBody(false) {}


Request::~Request() {}

void Request::printWithoutR(const std::string& what, const std::string& line) const {
    std::string l;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] != '\r')
            l.push_back(line[i]);
    }
    std::cout << what <<" = \'" << l << "\' -" << std::endl;
}

void Request::clearPreviousRequest() {
    reqServer = NULL, reqLocation = NULL;

    isCgi = false, returnDirective = false, _indexFound = false, isMultipart = false, \
    multipartRemaining = false, isChunked = false, remainingBody = false, \
    keepAlive = false, chunkRemaining = false;

    _failedUpload = 0, _totalUpload = 0;

    htmlPage.clear(), _headersStr.clear(), _requestLine.clear(), _body.clear(), headers.clear(), \
    chunk.clear(), trailing.clear(), scriptPath.clear(), queryString.clear(), _postExt.clear(), \
    _postFilename.clear(), _getPath.clear(), _multipartBoundary.clear(), _multipartContent.clear(), \
    fullBody.clear(), method.clear(), uri.clear(), version.clear(), uplaodFiles.clear(), \
    _multiTemp.headers.clear(), _multiTemp.body.clear(), _multiTemp.filename.clear(), _multiTemp.name.clear();
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

    while (start < end && (s[start] == ' ' || s[start] == '\t'))
        ++start;

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


void Request::findErrorPage(int code, const std::string& root, const std::map<int, std::string>& errPage) {

    err = true;
    status = code;

    std::map<int, std::string>::const_iterator itErr = errPage.find(code);
    if (itErr == errPage.end())
        return;

    std::string path;
    if (root[root.size() - 1] == '/' && itErr->second[0] == '/')
        path = root.substr(0, root.size() - 1) + itErr->second;
    else if (root[root.size() - 1] != '/' && itErr->second[0] != '/')
        path = root + "/" + itErr->second;
    else
        path = root + itErr->second;

    if (path[0] == '/')
        path = path.substr(1, path.size());

    Logger::debug("Finding Error Page at : " + path);
    
    std::ifstream file(path.c_str());
    if (!file) {
        return;
    } else {
        Logger::debug("Error Page Found");
        std::stringstream buffer;
        buffer << file.rdbuf();
        htmlPage = buffer.str();
    }
}

void Request::methodHandler() {
	if (method == "GET") {
        methodGetHandler();
	} else if (method == "DELETE") {
        methodDeleteHandler();
    } else if (method == "POST") {
        methodPostHandler();
    }
}



void Request::setServerInfo(const int& port, const std::string& ip) {
    _serverIp = ip;
    _serverPort = port;
}


bool Request::isCRLF(const std::string& request) {
    if (request.find("\r\n\r\n") != std::string::npos)
        return true;
    return false;
}