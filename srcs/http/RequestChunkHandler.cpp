#include "Request.hpp"

#include <iostream>
#include <cstdlib>

bool Request::getChunkSize() {
    // convert hexa to int
    char* end = NULL;
    std::string hex;
    size_t index = _chunk.find("\r\n");
    if (index == 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with chunked body = \\r\\n at index 0" << std::endl;
        return false;
    }
    hex = _chunk.substr(0, index);
    _chunk = _chunk.substr(index + 2, _chunk.size());
    _chunkSize = strtol(hex.c_str(), &end, 16);

    // std::cout << "CHUNK = " << _chunk << " VS HEX = " << hex << std::endl;
    std::cout << "CHUNK SIZE = " << _chunkSize << std::endl;

    if (end == hex.c_str()
            || *end != '\0'
            || _chunkSize < 0) { // what if overflow 
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with chunked size = erreur with hexa conversion" << std::endl;
        return false;
    }

    // set sate to reading
    _chunkState = READING_BYTES;
    return true;
}

void Request::parseChunk() {
    std::cout << "PARSE CHUNK = " << _body << std::endl;

    // Get 1st chunk size -> wait for \r\n
    if (_chunkState == GETTING_FIRST_SIZE) {
        if (_chunk.size() >= 2 &&
                _chunk.compare(_chunk.size() - 2, 2, "\r\n") == 0) {
            if (!getChunkSize())
                return ;
        } else {
            return ;
        }
        std::cout << "CHUNK = " << _chunk << std::endl;
    }


    // loop while chunk to read
    while (_chunkState != IS_END) {
        std::cout << "IN CHUNK LOOP" << std::endl;
        if (_reqLocation->bodySize < _body.size()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(413, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(413, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error body is higher that client max body size" << std::endl;
            return ;   
        }
        if (_chunkState == READING_BYTES) {

            std::cout << "READING BYTES\n" << std::endl;

            // get size bytes
            if ((_chunk.size() >= _chunkSize + 2)) {
                _body += _chunk.substr(0, _chunkSize);
                _chunk = _chunk.substr(_chunkSize, _chunk.size());
                std::cout << "CHUNK = " << _chunk << " VS BODY = " << _body << std::endl;
                size_t index = _chunk.find("\r\n");
                std::cout << "INDEX OF RN = " << index << std::endl;
                if (index != 0) {
                    if (!_reqLocation->root.empty())
                        findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                    else
                        findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                    std::cout << "error with chunked body = \\r\\n missing" << std::endl;
                    return ;
                }
                if (_chunkSize == 0 && _chunk == "\r\n") {
                    _chunkState = IS_END;
                    chunkRemaining = false;
                    return ;
                } else if (_chunkSize == 0) {
                    if (!_reqLocation->root.empty())
                        findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                    else
                        findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                    std::cout << "error with chunked body = end not well formated" << std::endl;
                    return ;
                }
                _chunk = _chunk.substr(index + 1, _chunk.size());
                std::cout << "CHUNK at the end of reading bytes = " << _chunk << " VS BODY = " << _body << std::endl;

                _chunkSize = -1;
                _chunkState = GETTING_SIZE;
            } else {
                return ;
            }
        } else if (_chunkState == GETTING_SIZE) {
            std::cout << "GETTING SIZE\n" << std::endl;

            if (_chunk.size() >= 2 &&
                    _chunk.compare(_chunk.size() - 2, 2, "\r\n") == 0) {
                if (!getChunkSize())
                    return ;
            } else {
                return ;
            }
            // std::cout << "CHUNK = " << _chunk << std::endl;
        } 
    }
}
