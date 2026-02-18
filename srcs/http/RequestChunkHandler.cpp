#include "Request.hpp"
#include "Logger.hpp"

#include <iostream>
#include <cstdlib>
#include <algorithm>

bool Request::convertHexa(std::string& hex) {
    std::string base = "0123456789abcdef";
    _chunkSize = 0;

    std::transform(hex.begin(), hex.end(), hex.begin(), ::tolower);
    size_t i = 0;
    for (; i != hex.size(); i++) {
        size_t index = base.find(hex[i]);
        if (index == std::string::npos)
            return false;
        _chunkSize = _chunkSize * 16 + index;
    }
    if (_chunkSize > __DBL_MAX__)
        return false;
    return true;
}


bool Request::getChunkSize() {
    std::string hex;
    size_t index = chunk.find("\r\n");
    if (index == 0) {
        if (!reqLocation->root.empty()) {
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Chunked Body: not well formatted");
        return false;
    }
    hex = chunk.substr(0, index);
    chunk = chunk.substr(index + 2, chunk.size());

    if (!convertHexa(hex)) {
        if (!reqLocation->root.empty()) {
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Chunked Body: hexa conversion failed");
        return false;
    }

    _chunkState = READING_BYTES;
    return true;
}

void Request::parseChunk() {

    if (_chunkState == GETTING_FIRST_SIZE) {
        if (chunk.size() >= 2 &&
                chunk.compare(chunk.size() - 2, 2, "\r\n") == 0) {
            if (!getChunkSize())
                return ;
        } else {
            return ;
        }
    }


    while (_chunkState != IS_END) {
        if (reqLocation->bodySize < _body.size()) {
            if (!reqLocation->root.empty()) {
                findErrorPage(413, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(413, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Chunked Body: Body size higher than client max body size");
            return ;
        }
        if (_chunkState == READING_BYTES) {

            if ((chunk.size() >= _chunkSize + 2)) {
                _body += chunk.substr(0, _chunkSize);
                chunk = chunk.substr(_chunkSize, chunk.size());

                size_t index = chunk.find("\r\n");

                if (index != 0) {
                    if (!reqLocation->root.empty())
                        findErrorPage(400, reqLocation->root, reqLocation->errPage);
                    else
                        findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                    Logger::warn("Chunked Body: not well formatted - missing CRLF");
                    return ;
                }

                if (_chunkSize == 0 && chunk == "\r\n") {
                    _chunkState = IS_END;
                    chunkRemaining = false;
                    fullBody = _body;
                    if (isMultipart) {
                        _multipartState = GETTING_FIRST_BOUNDARY;
                        fullBody = _body;
                        chunk = _body;
                        parseMultipart();
                    }
                    return ;

                } else if (_chunkSize == 0) {
                    if (!reqLocation->root.empty())
                        findErrorPage(400, reqLocation->root, reqLocation->errPage);
                    else
                        findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                    Logger::warn("Chunked Body: end not well formatted");
                    return ;
                }
                chunk = chunk.substr(index + 2, chunk.size());

                _chunkSize = -1;
                _chunkState = GETTING_SIZE;
            } else {
                return ;
            }
        } else if (_chunkState == GETTING_SIZE) {

            if (chunk.size() >= 2 &&
                    chunk.compare(chunk.size() - 2, 2, "\r\n") == 0) {
                if (!getChunkSize())
                    return ;
            } else {
                return ;
            }
        }
    }
}
