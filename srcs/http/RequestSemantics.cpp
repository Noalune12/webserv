#include <Request.hpp>
#include "Logger.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>

void Request::checkRequestSem(std::string request) {

    err = false;
    _req = request;

    if (_req.empty()) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request is empty");
        return ;
    }

    std::string methodTemp, uriTemp, http;

    if (!extractRequestInfo() || !extractRequestLineInfo(methodTemp, uriTemp, http)
            || !checkRequestLine(methodTemp, uriTemp, http) || !checkHeaders())
        return ;

    status = 200;
}

bool Request::extractRequestInfo() {

    // Extract Request Line

    size_t index = _req.find("\r\n");
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request not well formatted");
        return false;
    }

    _requestLine = _req.substr(0, index);
    _req = _req.substr(index + 2);

    // Extract Headers

    index = _req.find("\r\n\r\n");
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request not well formatted");
        return false;
    }

    _headersStr = _req.substr(0, index + 1);
    _req = _req.substr(index + 4);

    // Extract body
    _body = _req;

    return true;
}


 bool Request::extractRequestLineInfo(std::string& methodTemo, std::string& uriTemp, std::string& http) {

    // method

    size_t index = _requestLine.find(' ');
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request Line not well formatted");
        return false;
    }

    methodTemo = _requestLine.substr(0, index);
    std::string remain = _requestLine.substr(index + 1, _requestLine.length());

    // uri

    index = remain.find(' ');
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request Line not well formatted");
        return false;
    }

    uriTemp = remain.substr(0, index);
    remain = remain.substr(index + 1, remain.length());

    //http

    if (remain.empty()) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request Line not well formatted");
        return false;
    }

    http = remain;

    return true;
}

bool Request::checkHeaders() {
    std::stringstream ss(_headersStr);
    std::string line;

    while (std::getline(ss, line)) {
        if (line[line.length() - 1] != '\r') {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers not well formatted");
            return false;
        }

        line = line.substr(0, line.length() - 1);
        size_t index = line.find(":");
        if (index == 0 || index == std::string::npos) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers not well formatted");
            return false;
        }
        std::string name = line.substr(0, index);
        if (hasWS(name)) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers not well formatted");
            return false;
        }


        std::string content = line.substr(index + 1);

        if (!content.empty() && (content[0] != ' ' && content[0] != '\t')) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers not well formatted");
            return false;
        }

        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        content = trimOws(content);
        if (name != "user-agent" && name != "content-type")
            std::transform(content.begin(), content.end(), content.begin(), ::tolower);

        if ((name == "host" && headers.find("host") != headers.end())
                || (name == "user-agent" && headers.find("user-agent") != headers.end())
                || (name == "content-length" && headers.find("content-length") != headers.end())
                || (name == "transfer-encoding" && headers.find("transfer-encoding") != headers.end())
                || (name == "content-type" && headers.find("content-type") != headers.end())
                || (name == "connection" && headers.find("connection") != headers.end())) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers " + name + " is duplicated");
            return false;
        }
        if ((name == "host") && hasWS(content)) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers: Host content has White Spaces");
            return false;
        }
        if (((name == "host") && content.empty())
                || ((name == "content-length") && content.empty())
                || ((name == "transfer-encoding") && content.empty())
                || ((name == "content-type") && content.empty())
                || ((name == "connection") && content.empty())) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers " + name + " can not have an empty content");
            return false;
        }
        if (name == "content-length" && !isOnlyDigits(content)) {
            findErrorPage(400, "/", _globalDir.errPage);
            Logger::warn("Headers: Content Length is not well formatted");
            return false;
        }
        headers[name] = content;
    }

    return true;
}

static bool isValidHTTPVersion(const std::string& version) {

    if (version.size() != 3)
        return (false);

    return (std::isdigit(version[0]) && version[1] == '.' && std::isdigit(version[2]));
}

bool Request::checkRequestLine(const std::string& methodTemp, const std::string& uriTemp, const std::string& http) {
    if (methodTemp != "GET" && methodTemp != "POST" && methodTemp != "DELETE"
            && methodTemp != "HEAD" && methodTemp != "OPTIONS"
            && methodTemp != "TRACE" && methodTemp != "PUT"
            && methodTemp != "PATCH" && methodTemp != "CONNECT") {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request Line: Method is wrong");
        return false;
    }
    if (uriTemp[0] != '/') {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request Line: Uri is wrong");
        return false;
    }
    size_t index = http.find ("HTTP/");
    if (index != 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request Line: HTTP is wrong");
        return false;
    }

    std::string versionTemp = http.substr(5);
    if (!isValidHTTPVersion(versionTemp)) {
        findErrorPage(400, "/", _globalDir.errPage);
        Logger::warn("Request Line: HTTP has wrong syntaxe");
        return false;
    }
    if (versionTemp != "1.1" && versionTemp != "1.0") {
        findErrorPage(505, "/", _globalDir.errPage);
        Logger::warn("Request Line: wrong HTTP version");
        return false;
    }
    method = methodTemp;
    uri = uriTemp;
    version = versionTemp;
    return true;
}
