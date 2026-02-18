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
    std::map<std::string, std::string>::iterator it = headers.find("content-type");

    if (it != headers.end()) {

        checkMultipart(it->second);

        if (isMultipart && !chunkRemaining) {

            it = headers.find("content-length");

            if (it == headers.end()) {
                if (!reqLocation->root.empty()) {
                    findErrorPage(411, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(411, reqLocation->alias, reqLocation->errPage);
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
    std::map<std::string, std::string>::iterator it = headers.find("transfer-encoding");

    if (it != headers.end() && it->second != "chunked") {
        if (!reqLocation->root.empty())
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        else
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        Logger::warn("Transfer Encoding not well defined");
        return false;
    } else if (it != headers.end() && it->second == "chunked") {
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

    if (reqLocation->bodySize < body.size()) {
        if (!reqLocation->root.empty())
            findErrorPage(413, reqLocation->root, reqLocation->errPage);
        else
            findErrorPage(413, reqLocation->alias, reqLocation->errPage);
        Logger::warn("Body size higher than client max body size");
        return false;
    }

    std::map<std::string, std::string>::iterator it = headers.find("content-length");
    if (it == headers.end() && !body.empty()) {
        if (!reqLocation->root.empty()) {
            findErrorPage(411, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(411, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Content Length is missing");
        return false;

    } else if (it != headers.end()) {

        std::stringstream ss(it->second);
        double contentLength = 0;
        ss >> contentLength;

        if (ss.fail()) {
            if (!reqLocation->root.empty()) { 
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
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
                if (!reqLocation->root.empty()) {
                    findErrorPage(400, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Body size differs from Content Lentgh");
                return false;
            }
        } else {
            if (bodySize < contentLength) {
                remainingBody = true;
                fullBody = body;
            } else if (bodySize > contentLength) {
                if (!reqLocation->root.empty()) {
                    findErrorPage(400, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Body size differs from Content Lentgh");
                return false;
            } else if (bodySize == contentLength) {
                fullBody = body;
            }
        }

    }

    return true;

}
