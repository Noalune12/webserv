#include "Request.hpp"
#include "Logger.hpp"

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

        if (isMultipart && !chunkRemaining) {

            it = _headers.find("content-length");

            if (it == _headers.end()) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(411, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(411, _reqLocation->alias, _reqLocation->errPage);
                }
                Logger::warn("Multipart: Content-Length missing");
                return false;
            }

            _multipartState = GETTING_FIRST_BOUNDARY;
            chunk = _body;
            fullBody = _body;

            if (!parseMultipart())
                return false;
            return true;
        } else {
            std::transform(it->second.begin(), it->second.end(), it->second.begin(), ::tolower);
        }
    }

    if (isChunked)
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
        Logger::warn("Transfer Encoding not well defined");
        return false;
    } else if (it != _headers.end() && it->second == "chunked") {
        Logger::debug("Body is Chunked");
        isChunked = true;
        chunkRemaining = true;
        chunk = _body;
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
        Logger::warn("Body size higher than client max body size");
        return false;
    }

    std::map<std::string, std::string>::iterator it = _headers.find("content-length");
    if (it == _headers.end() && !body.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(411, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(411, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Content Length is missing");
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
            Logger::warn("Content Length can't be converted");
            return false;
        }

        double bodySize = static_cast<double>(body.size());

        if (remainingBody == true) {
            if (bodySize == contentLength) {
                remainingBody = false;
                return true;
            } else if (bodySize > contentLength) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                Logger::warn("Body size differs from Content Lentgh");
                return false;
            }
        } else {
            if (bodySize < contentLength) {
                remainingBody = true;
                fullBody = _body;
            } else if (bodySize > contentLength) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                Logger::warn("Body size differs from Content Lentgh");
                return false;
            } else if (bodySize == contentLength) {
                fullBody = _body;
            }
        }

    }

    return true;

}
