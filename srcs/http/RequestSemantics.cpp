#include <Request.hpp>

#include <iostream>
#include <algorithm>
#include <sstream>

void Request::checkRequestSem(std::string request) {
    
    err = false;
    _req = request;

    if (_req.empty()) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error empty request" << std::endl;
        return ;
    }

    std::string method, uri, http;

    if (!extractRequestInfo() || !extractRequestLineInfo(method, uri, http)
            || !checkRequestLine(method, uri, http) || !checkHeaders())
        return ;


    printWithoutR("METHOD", _method);
    printWithoutR("URI", _uri);
    printWithoutR("BODY", _body);

    status = 200;
}

bool Request::extractRequestInfo() {

    // Extract Request Line

    size_t index = _req.find("\r\n");
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with request line : not finished with rn" << std::endl;
        return false;  
    }

    _requestLine = _req.substr(0, index);
    _req = _req.substr(index + 2);

    // Extract Headers

    index = _req.find("\r\n\r\n");
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error no header or final WS" << std::endl;
        return false;  
    }

    _headersStr = _req.substr(0, index + 1);
    _req = _req.substr(index + 4);

    // Extract body
    _body = _req;
    
    return true;
}


 bool Request::extractRequestLineInfo(std::string& method, std::string& uri, std::string& http) {
    
    // method
    
    size_t index = _requestLine.find(' ');
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with request line: method" << std::endl;
        return false;
    }

    method = _requestLine.substr(0, index);
    std::string remain = _requestLine.substr(index + 1, _requestLine.length());

    // uri

    index = remain.find(' ');
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with request line: uri" << std::endl;
        return false;
    }

    uri = remain.substr(0, index);
    remain = remain.substr(index + 1, remain.length());

    //http

    if (remain.empty()) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with request line: http" << std::endl;
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
            std::cout << "error with headers no \\r at the end" << std::endl;
            return false;
        }

        line = line.substr(0, line.length() - 1);
        size_t index = line.find(":");
        if (index == 0 || index == std::string::npos) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error with headers no : or no header name" << std::endl;
            return false;
        }
        std::string name = line.substr(0, index);
        if (hasWS(name)) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error with headers name has WS" << std::endl;
            return false;
        }


        std::string content = line.substr(index + 1);
        
        if (!content.empty() && (content[0] != ' ' && content[0] != '\t')) {
            std::cout << "CONTENT = \'" << content << "\'" << std::endl;
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error with headers : is not followed by space" << std::endl;
            return false;
        }

        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        content = trimOws(content);
        if (name != "user-agent" && name != "content-type")
            std::transform(content.begin(), content.end(), content.begin(), ::tolower);
        
        std::cout << "NAME, CONTENT FOR HEADER = " << name << ", " << content << std::endl;
        if ((name == "host" && _headers.find("host") != _headers.end()) 
                || (name == "user-agent" && _headers.find("user-agent") != _headers.end())
                || (name == "content-length" && _headers.find("content-length") != _headers.end())
                || (name == "transfer-encoding" && _headers.find("transfer-encoding") != _headers.end())
                || (name == "content-type" && _headers.find("content-type") != _headers.end())
                || (name == "connection" && _headers.find("connection") != _headers.end())) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error with duplicate headers : " << name << std::endl;
            return false;  
        }
        if ((name == "host") && hasWS(content)) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error header : " << name << " has WS in its content" << std::endl;
            return false;
        }
        if (((name == "host") && content.empty())
                || ((name == "content-length") && content.empty())
                || ((name == "transfer-encoding") && content.empty())
                || ((name == "content-type") && content.empty())
                || ((name == "connection") && content.empty())) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error header : " << name << " cannot have an empty content" << std::endl;
            return false;
        }
        if (name == "content-length" && !isOnlyDigits(content)) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error header : " << name << " has to be only digits" << std::endl;
            return false;
        }
        _headers[name] = content;
    }

    std::cout << "\nHEADER MAP\n" << std::endl;
    std::map<std::string, std::string>::iterator it = _headers.begin();
    for (; it != _headers.end(); it++) {
        std::cout << "[" << it->first << ", " << it->second << "]" << std::endl;
    }

    return true;
}

bool Request::checkRequestLine(std::string& method, std::string& uri, std::string& http) {
    if (method != "GET" && method != "POST" && method != "DELETE"
            && method != "HEAD" && method != "OPTIONS"
            && method != "TRACE" && method != "PUT"
            && method != "PATCH" && method != "CONNECT") {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with method" << std::endl;
        return false;
    }
    if (uri[0] != '/') { 
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with uri" << std::endl;
        return false;
    }
    size_t index = http.find ("HTTP/");
    if (index != 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with http index is not 0" << std::endl;
        return false;
    }
    std::string version = http.substr(index + 5, http.length());
    std::cout << "VERSION = " << version << std::endl;
    if (version == "2" || version == "3") {
        findErrorPage(505, "/", _globalDir.errPage);
        std::cout << "error with http not the real version" << std::endl;
        return false;
    }
    if (version != "1.1" && version != "1.0") {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with http" << std::endl;
        return false;
    }
    _method = method;
    _uri = uri;
    return true;
}