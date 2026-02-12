#include "Request.hpp"
#include "Logger.hpp"

#include <iostream>
#include <cstdlib>

bool Request::getChunkSize() {
    Logger::debug("Chunked Body: Getting Size");
    char* end = NULL;
    std::string hex;
    size_t index = _chunk.find("\r\n");
    if (index == 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Chunked Body: not well formatted");
        return false;
    }
    hex = _chunk.substr(0, index);
    _chunk = _chunk.substr(index + 2, _chunk.size());
    _chunkSize = strtol(hex.c_str(), &end, 16);

    if (end == hex.c_str()
            || *end != '\0'
            || _chunkSize < 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Chunked Body: hexa conversion failed");
        return false;
    }

    Logger::debug("Chunked body size: "), std::cout << _chunkSize << std::endl;;
    _chunkState = READING_BYTES;
    return true;
}

void Request::parseChunk() {

    if (_chunkState == GETTING_FIRST_SIZE) {
        if (_chunk.size() >= 2 &&
                _chunk.compare(_chunk.size() - 2, 2, "\r\n") == 0) {
            if (!getChunkSize())
                return ;
        } else {
            return ;
        }
    }


    while (_chunkState != IS_END) {
        if (_reqLocation->bodySize < _body.size()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(413, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(413, _reqLocation->alias, _reqLocation->errPage);
            }
            Logger::warn("Chunked Body: Body size higher than client max body size");
            return ;   
        }
        if (_chunkState == READING_BYTES) {

            Logger::debug("Chunked Body: Reading Bytes");
            if ((_chunk.size() >= _chunkSize + 2)) {
                _body += _chunk.substr(0, _chunkSize);
                _chunk = _chunk.substr(_chunkSize, _chunk.size());

                size_t index = _chunk.find("\r\n");

                if (index != 0) {
                    if (!_reqLocation->root.empty())
                        findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                    else
                        findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                    Logger::warn("Chunked Body: not well formatted - missing CRLF");
                    return ;
                }

                if (_chunkSize == 0 && _chunk == "\r\n") {
                    Logger::debug("Chunked Body: End found");
                    _chunkState = IS_END;
                    chunkRemaining = false;
                    if (_isMultipart) {
                        _multipartState = GETTING_FIRST_BOUNDARY;
                        _fullBody = _body;
                        _chunk = _body;
                        parseMultipart();
                    }
                    return ;

                } else if (_chunkSize == 0) {
                    if (!_reqLocation->root.empty())
                        findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                    else
                        findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                    Logger::warn("Chunked Body: end not well formatted");
                    return ;
                }
                _chunk = _chunk.substr(index + 1, _chunk.size());

                _chunkSize = -1;
                _chunkState = GETTING_SIZE;
            } else {
                return ;
            }
        } else if (_chunkState == GETTING_SIZE) {

            if (_chunk.size() >= 2 &&
                    _chunk.compare(_chunk.size() - 2, 2, "\r\n") == 0) {
                if (!getChunkSize())
                    return ;
            } else {
                return ;
            }
        } 
    }
}
