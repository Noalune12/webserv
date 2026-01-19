#include "Request.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <climits>

Request::Request(): err(false), status(0), chunkRemaining(false) {}

Request::Request(std::vector<server> servers, globalDir globalDir) : _servers(servers), _globalDir(globalDir), err(false), status(0), chunkRemaining(false) {}


Request::~Request() {}

static void printWithoutR(std::string what, std::string line) {
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
}

void Request::checkRequestSem(std::string request) {
    err = false;
    _req = request;
    if (_req.empty()) { // how to test
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error empty request" << std::endl;
        return ;
    }
    std::string method, uri, http;

    if (!extractRequestInfo() || !extractRequestLineInfo(method, uri, http)
            || !checkRequestLine(method, uri, http) || !checkHeaders())
        return ;

    // there can only be one host, content-length and user-agent
    // if transfer-encoding: chunked no content-length -> body finish with \r\n\r\n and then next request
    // compare body with content-length

    status = 200;
}

bool Request::extractRequestInfo() {
    // extracts request line, headers and body
    // std::string requestLine, headers;

    // Get request line
    size_t index = _req.find("\r\n");
    if (index == std::string::npos || index == 0) {
        // err = true;
        // status = 400; // need global dir if 400 is defines of need to get the host as well ?
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with request line : not finished with rn" << std::endl;
        return false;  
    }
    std::cout << "INDEX of r n = " << index << std::endl;
    _requestLine = _req.substr(0, index);
    _req = _req.substr(index + 2);
    printWithoutR("REQUEST LINE", _requestLine);
    printWithoutR("REMAINING REQUEST", _req);

    // Get headers
    index = _req.find("\r\n\r\n");
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error no header or final WS" << std::endl;
        return false;  
    }
    _headersStr = _req.substr(0, index + 1);
    _req = _req.substr(index + 4);
    printWithoutR("HEADERS", _headersStr);
    printWithoutR("REMAINING REQUEST", _req);

    // Get Body
    _body = _req;
    printWithoutR("BODY", _body);
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
    printWithoutR("METHOD", method);
    printWithoutR("REMAIN", remain);

    // uri
    index = remain.find(' ');
    if (index == std::string::npos || index == 0) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with request line: uri" << std::endl;
        return false;
    }
    uri = remain.substr(0, index);
    remain = remain.substr(index + 1, remain.length());
    printWithoutR("URI", uri);
    printWithoutR("REMAIN", remain);

    //http
    if (remain.empty()) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error with request line: http" << std::endl;
        return false;
    }
    http = remain;
    printWithoutR("HTTP", http);
    return true;

 }

bool hasWS(std::string s) {
    for (size_t i = 0; i < s.length(); i++) {
        if (isspace(s[i]))
            return true;
    }
    return false;
}



//if body -> check content-length is same

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
        // std::cout << "index : = " << index << std::endl;
        // std::cout << "HEADER = " << line << std::endl;
        if (index == 0 || index == std::string::npos || index == line.length() - 2) {
            findErrorPage(400, "/", _globalDir.errPage);
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
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error with duplicate headers : " << name << std::endl;
            return false;  
        }
        if (name == "host" && hasWS(content)) {
            findErrorPage(400, "/", _globalDir.errPage);
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

std::string Request::lowerString(const std::string& s) {
    std::string ret = s;
    for (std::string::size_type i = 0; i < ret.size(); ++i)
        ret[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ret[i]))
        );
    return ret;
}


void Request::checkRequestContent() {

    std::cout << "\nCHECK REQUEST CONTENT\n" << std::endl;
    // Host check
	std::map<std::string, std::string>::iterator it = _headers.find("host");
	if (it == _headers.end()) {
		findErrorPage(400, "/", _globalDir.errPage);
	    std::cout << "error no host in headers " << std::endl;
		return ;
	}
	size_t sep = it->second.find(":");
	if (sep == 0 || sep == it->second.size() - 1) {
		findErrorPage(400, "/", _globalDir.errPage);
	    std::cout << "error host has : at the start or the end" << std::endl;
		return ;
	}
	std::string name;
	int port;
	if (sep == std::string::npos) {
		name = it->second;
		port = 8080;
	} else {
		name = it->second.substr(0, sep);
		std::string temp = it->second.substr(sep + 1, it->second.size());
		std::stringstream ss(temp);
		ss >> port; // what if overflow
	}
	if (name == "localhost") {
		name = "127.0.0.1";
	}
	std::vector<server>::iterator itServer = _servers.begin();
	bool serverFound = false;
	for (; itServer != _servers.end(); itServer++) {
		std::vector<listenDirective>::iterator itListen = itServer->lis.begin();
		for (; itListen != itServer->lis.end(); itListen++) {
			if (itListen->port == port && (itListen->ip == name || itListen->ip == "0.0.0.0")) {
				// what about servername and virtual hosting
				std::cout << "server found with " << itListen->port << ", " << itListen->ip << std::endl;
				_reqServer = *itServer;
				serverFound = true;
				break;
			}
		}
		if (serverFound)
			break ;
	}

	if (itServer == _servers.end()) {
		findErrorPage(400, "/", _globalDir.errPage);
	    std::cout << "error no compatible server found" << std::endl;
		return ;
	}

	// uri check
	std::vector<location>::iterator itLocation = _reqServer.loc.begin();
	for (; itLocation != _reqServer.loc.end(); itLocation++) {
		if (_uri == itLocation->path) {
			std::cout << "location found at " << itLocation->path << std::endl;
			_reqLocation = *itLocation;
			break;
		}
	}

	if (itLocation == _reqServer.loc.end()) {
        findErrorPage(404, _reqServer.root, _reqServer.errPage);
	    std::cout << "error no location found" << std::endl;
		return ;
	}

	// method check
	if ((_method == "GET" && _reqLocation.methods.get == false)
			|| (_method == "POST" && _reqLocation.methods.post == false)
            || (_method == "DELETE" && _reqLocation.methods.del == false)
            || _method == "HEAD" || _method == "OPTIONS"
            || _method == "TRACE" || _method == "PUT"
            || _method == "PATCH" || _method == "CONNECT") {
        findErrorPage(405, _reqLocation.root, _reqLocation.errPage);
	    std::cout << "error not allowed method" << std::endl;
		return ;
	}

	// body check + content max body size
    // find transfer encoding
    it = _headers.find("transfer-encoding");
    if (it != _headers.end() && it->second != "chunked") {
        findErrorPage(400, _reqLocation.root, _reqLocation.errPage);
        std::cout << "error transfer encoding not well defined" << std::endl;
        return ;
    } else if (it != _headers.end() && it->second == "chunked") {
        std::cout << "PARSE CHUNKED BODY HERE" << std::endl;
        // get line (1st bytes then compare with size of second -> sep are \r\n, chunked is finished if 0/r/n/r/n what if there are remaining stuff)

        // bool isSize = true;
        // first check that we have the end and remove it with compare
        if (_body.size() >= 5 &&
                _body.compare(_body.size() - 5, 5, "0\r\n\r\n") == 0) {
                chunkRemaining = false;
                _body = _body.substr(0, _body.size() - 5);
                // CHUNK PARSER
        } else {
            chunkRemaining = true;
        }


        // add a check if last line does not hace a \n
        // if (_body.size() >= 2 &&
        //         _body.compare(_body.size() - 2, 2, "\r\n") == 0) {
        //         _body = _body.substr(0, _body.size() - 2);
        // } else {
        //     findErrorPage(400, _reqLocation.root, _reqLocation.errPage);
        //     std::cout << "error with chunked body = body is not well ended" << std::endl;
        //     return ;
        // }
        // std::stringstream ss(_body);
        // std::string line;
        // std::cout << "CHUNKED BODY = \'" << _body << "\'" << std::endl;
        // double chunkSize;
        // std::string chunkContent, temp;
        // while (std::getline(ss, line)) {

        //     std::cout << "LINE = " << line << std::endl;

        //     // compare with max body size

        //     if (_reqLocation.bodySize < chunkContent.size()) {
        //         findErrorPage(413, _reqLocation.root, _reqLocation.errPage);
        //         std::cout << "error body is higher that client max body size" << std::endl;
        //         return ;   
        //     }

        //     if (isSize == true) {
        //         // convert hexa to int
        //         char* end = NULL;
        //         std::string hex;
        //         size_t index = line.find('\r');
        //         if (index == 0 || index == std::string::npos) {
        //             findErrorPage(400, _reqLocation.root, _reqLocation.errPage);
        //             std::cout << "error with chunked body = \\r not found or at index 0" << std::endl;
        //             return ;
        //         }
        //         hex = line.substr(0, index);
        //         chunkSize = strtol(hex.c_str(), &end, 16);

        //         if (end == hex.c_str()
        //                 || *end != '\0'
        //                 || chunkSize < 0) { // what if overflow 
        //             findErrorPage(400, _reqLocation.root, _reqLocation.errPage);
        //             std::cout << "error with chunked body = erreur with hexa conversion" << std::endl;
        //             return ;
        //         }
        //         std::cout << "CHUNK SIZE = " << chunkSize << std::endl;
        //         isSize = false;
        //     } else {
        //         // get _body until \r
        //         size_t index = line.find('\r');
        //         if (index == std::string::npos) {
        //             temp += line;
        //             temp += '\n';
        //             std::cout << "TEMP = \'" << temp << "\'" << std::endl; 
        //             continue;
        //         } else {
        //             // compare with actual size
        //             line = line.substr(0, index);
        //             temp += line;
        //             std::cout << "TEMP JUST BEFORE CHECK = \'" << temp << "\'" << std::endl; 
        //             if (temp.size() != chunkSize) {
        //                 findErrorPage(400, _reqLocation.root, _reqLocation.errPage);
        //                 std::cout << "error with chunked body = not the right size" << std::endl;
        //                 return ;
        //             }
        //             chunkContent += temp;
        //             std::cout << "CHUNK = \'" << chunkContent << "\'" << std::endl; 
        //             isSize = true;
        //             temp.clear();
        //         }
        //     }
        // }
        // _body = chunkContent; 
        // // change logic get the hexa octets
        // // check if ending or if I will receie several chunk
        // // parse chunk in a loop (to define)
        //     // get size
        //     // read size octet
        //     // if I was supposed to get the whole chunks or save size if I will receive more
        //         // partChunk + chunkContent
            
    } else if (it == _headers.end()) {
        if (_reqLocation.bodySize < _body.size()) {
            findErrorPage(413, _reqLocation.root, _reqLocation.errPage);
            std::cout << "error body is higher that client max body size" << std::endl;
            return ;   
        }

        // find content length
        
        it = _headers.find("content-length");
        if (it == _headers.end() && !_body.empty()) {
            findErrorPage(400, _reqLocation.root, _reqLocation.errPage);
            std::cout << "error no content length but existing body" << std::endl;
            return ;
        } else if (it != _headers.end()) {
            std::cout << "CONTENT LENGTH = " << it->second << std::endl;
            std::stringstream ss;
            ss << _body.size();
            std::string bodySize = ss.str();
            std::cout << "BODY SIZE = " << bodySize << std::endl;
            if (it->second != bodySize) {
                findErrorPage(400, _reqLocation.root, _reqLocation.errPage);
                std::cout << "error  content length is not equal to existing body size" << std::endl;
                return ;
            }
        }
    }
    // if method is POST but no body => 400
    // regarding content type -> parse the body ?

	// get info
	if (_method == "GET") {
		std::vector<std::string>::iterator itIndex = _reqLocation.index.begin();
		std::string root = _reqLocation.root; // what if no root ? if starts with / need to check ?
		for (; itIndex != _reqLocation.index.end() ; itIndex++) {
			std::string path = root + _uri + *itIndex; // what if directory does not exist ...
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
	
		if (htmlPage.empty()) {
            findErrorPage(403, _reqLocation.root, _reqLocation.errPage);
			std::cout << "error no index found" << std::endl;
			return ;
		}
	}
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

void Request::parseChunk() {
    std::cout << "PARSE CHUNK = " << _body << std::endl;
}


void Request::isChunkEnd() {
    if (_body.size() >= 5 &&
            _body.compare(_body.size() - 5, 5, "0\r\n\r\n") == 0) {
            std::cout << "CHECKING END OF CHUNK = " << _body << std::endl;;
            chunkRemaining = false;
            _body = _body.substr(0, _body.size() - 5);
    } else {
        chunkRemaining = true;
    }
}