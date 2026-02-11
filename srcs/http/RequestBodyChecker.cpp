#include "Request.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>

bool Request::bodyChecker() {

    // Check if transfer-encoding: chunked
    if (!checkChunked())
        return false;

    // check if multipart
    std::map<std::string, std::string>::iterator it = _headers.find("content-type");

    if (it != _headers.end()) {

        checkMultipart(it->second);

        if (_isMultipart && !chunkRemaining) {

            it = _headers.find("content-length");

            if (it == _headers.end()) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(411, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(411, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error no content length but existing body for multipart" << std::endl;
                return false;
            }

            _multipartState = GETTING_FIRST_BOUNDARY;
            _chunk = _body;
            _fullBody = _body;

            if (!parseMultipart())
                return false;
            return true;
        } else {
            std::transform(it->second.begin(), it->second.end(), it->second.begin(), ::tolower);
        }
    }

    if (_isChunked)
         return true;

    if (!checkBodySize(_body))
        return false;
    
    return true;
}

bool Request::checkChunked() {
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
        _isChunked = true;
        chunkRemaining = true;
        _chunk = _body;
        _body.clear();
        _chunkState = GETTING_FIRST_SIZE;
        parseChunk();
        return true;
    }
    return true;
}

bool Request::checkBodySize(const std::string &body) { 

    if (_reqLocation->bodySize < body.size()) {
        if (!_reqLocation->root.empty())
            findErrorPage(413, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(413, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error body is higher that client max body size" << std::endl;
        return false;
    }

    std::map<std::string, std::string>::iterator it = _headers.find("content-length");
    if (it == _headers.end() && !body.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(411, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(411, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error no content length but existing body" << std::endl;
        return false;

    } else if (it != _headers.end()) {

        std::stringstream ss(it->second);
        double contentLength = 0;
        ss >> contentLength;

        if (ss.fail()) {
            if (!_reqLocation->root.empty()) { 
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error  while converting content len" << std::endl;
            return false;
        }

        double bodySize = static_cast<double>(body.size());

        if (_remainingBody == true) {
            if (bodySize == contentLength) {
                _remainingBody = false;
                return true;
            } else if (bodySize > contentLength) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error  content length is not equal to existing body size" << std::endl;
                return false;
            }
        } else {
            if (bodySize < contentLength) {
                _remainingBody = true;
                _fullBody = _body;
            } else if (bodySize > contentLength) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error  content length is not equal to existing body size" << std::endl;
                return false;
            } else if (bodySize == contentLength) {
                _fullBody = _body;
            }
        }

    }

    return true;

}
