#include "RequestParsing.hpp"

#include <iostream>
#include <sstream>

RequestParsing::RequestParsing(): err(false), status(0) {}


RequestParsing::~RequestParsing() {}

static void printWithoutR(std::string what, std::string line) {
    std::string l;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] != '\r')
            l.push_back(line[i]);
    }
    std::cout << what <<" = \'" << l << "\' -" << std::endl;
}

void RequestParsing::checkRequestSem(std::string request) {
    _req = request;
    if (_req.empty()) { // how to test
        err = true;
        status = 400;
        std::cout << "error empty request" << std::endl;
        return ;
    }

    // extracts request line, headers and body
    std::string requestLine, headers;
    size_t index = _req.find("\r\n");
    if (index == std::string::npos || index == 0) {
        err = true;
        status = 400;
        std::cout << "error with request line" << std::endl;
        return ;  
    }
    std::cout << "INDEX of r n = " << index << std::endl;
    requestLine = _req.substr(0, index);
    _req = _req.substr(index + 2);
    printWithoutR("REQUEST LINE", requestLine);
    printWithoutR("REMAINING REQUEST", _req);

    index = _req.find("\r\n\r\n");
    if (index == std::string::npos || index == 0) {
        err = true;
        status = 400;
        std::cout << "error no header or final WS" << std::endl;
        return ;  
    }
    headers = _req.substr(0, index + 1);
    _req = _req.substr(index + 4);
    printWithoutR("HEADERS", headers);
    printWithoutR("REMAINING REQUEST", _req);

    _body = _req;
    printWithoutR("BODY", _body);

    // ANALYSE REQUEST LINE
    std::string method, uri, http, host;

    // method
    index = requestLine.find(' ');
    if (index == std::string::npos || index == 0) {
        err = true;
        status = 400;
        std::cout << "error with request line" << std::endl;
        return ;
    }
    method = requestLine.substr(0, index);
    std::string remain = requestLine.substr(index + 1, requestLine.length());
    printWithoutR("METHOD", method);
    printWithoutR("REMAIN", remain);

    // uri
    index = remain.find(' ');
    if (index == std::string::npos || index == 0) {
        err = true;
        status = 400;
        std::cout << "error with request line" << std::endl;
        return ;
    }
    uri = remain.substr(0, index);
    remain = remain.substr(index + 1, remain.length());
    printWithoutR("URI", uri);
    printWithoutR("REMAIN", remain);

    //http
    if (remain.empty()) {
        err = true;
        status = 400;
        std::cout << "error with request line" << std::endl;
        return ;
    }
    http = remain;
    printWithoutR("HTTP", http);

    std::cout << "request line is ok" << std::endl;

    if (!checkRequestLine(method, uri, http) || !checkHeaders(headers)) {
        return ;
    }

    // there can only be one host, content-length and user-agent
    // if transfer-encoding: chunked no content-length -> body finish with \r\n\r\n and then next request
    // compare body with content-length

    status = 200;
}

bool hasWS(std::string s) {
    for (size_t i = 0; i < s.length(); i++) {
        if (isspace(s[i]))
            return true;
    }
    return false;
}

//if body -> check content-length is same

bool RequestParsing::checkHeaders(std::string headers) {
    _headers.clear();
    std::stringstream ss(headers);
    std::string line;
    while (std::getline(ss, line)) {
        if (line[line.length() - 1] != '\r') {
            err = true;
            status = 400;
            std::cout << "error with headers no \\r at the end" << std::endl;
            return false;
        }
        line = line.substr(0, line.length() - 1);
        size_t index = line.find(":");
        // std::cout << "index : = " << index << std::endl;
        // std::cout << "HEADER = " << line << std::endl;
        if (index == 0 || index == std::string::npos || index == line.length() - 2) {
            err = true;
            status = 400;
            std::cout << "error with headers no : or no header name or content" << std::endl;
            return false;
        }
        std::string name = line.substr(0, index);
        std::string content = line.substr(index + 2);
        // skip whitespace afte :
        name = lowerString(name);
        content = trimOws(content);
        if (name != "user-agent")
            content = lowerString(content);
        
        std::cout << "NAME, CONTENT FOR HEADER = " << name << ", " << content << std::endl;
        if ((name == "host" && _headers.find("host") != _headers.end()) 
                || (name == "user-agent" && _headers.find("user-agent") != _headers.end())
                || (name == "content-length" && _headers.find("content-length") != _headers.end())
                || (name == "transfer-encoding" && _headers.find("transfer-encoding") != _headers.end())) {
            err = true;
            status = 400;
            std::cout << "error with duplicate headers : " << name << std::endl;
            return false;  
        }
        if (name == "host" && hasWS(content)) {
            err = true;
            status = 400;
            std::cout << "error header : " << name << " has WS in its content" << std::endl;
            return false;
        }
        _headers[name] = content;
    }

    std::cout << "\nHEADER MAP\n" << std::endl;
    std::map<std::string, std::string>::iterator it = _headers.begin();
    for (; it != _headers.end(); it++) {
        std::cout << "[" << it->first << ", " << it->second << "]" << std::endl;
    }
    // has host
    return true;
}

bool RequestParsing::checkRequestLine(std::string& method, std::string& uri, std::string& http) {
    if (method != "GET" && method != "POST" && method != "DELETE") { //what if method is not accepted
        err = true;
        status = 400;
        std::cout << "error with method" << std::endl;
        return false;
    }
    if (uri[0] != '/') { //what if uri is not defined
        err = true;
        status = 400;
        std::cout << "error with uri" << std::endl;
        return false;
    }
    if (http != "HTTP/1.1") {
        err = true;
        status = 400;
        std::cout << "error with http" << std::endl;
        return false;
    }
    _method = method;
    _uri = uri;
    return true;
}

std::string RequestParsing::trimOws(const std::string& s)
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

std::string RequestParsing::lowerString(const std::string& s) {
    std::string ret = s;
    for (std::string::size_type i = 0; i < ret.size(); ++i)
        ret[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ret[i]))
        );
    return ret;
}
