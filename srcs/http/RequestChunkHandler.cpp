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

// static void printStr(std::string str) {
//     size_t i = 0;
//     for (; i != str.size(); i++) {
//         if (str[i] == '\n')
//             std::cout << "\\n";
//         if (str[i] == '\r')
//             std::cout << "\\r";
//         else
//             std::cout << str[i];
//     }
// }

bool Request::getChunkSize() {
    Logger::debug("Chunked Body: Getting Size");
    // char* end = NULL;
    std::string hex;
    size_t index = chunk.find("\r\n");
    if (index == 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Chunked Body: not well formatted");
        return false;
    }
    hex = chunk.substr(0, index);
    chunk = chunk.substr(index + 2, chunk.size());

    // std::cout << "Hex = ", printStr(hex), std::cout << " \nand chunk is : ", printStr(_chunk), std::cout << std::endl;

    if (!convertHexa(hex)) {
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
        if (chunk.size() >= 2 &&
                chunk.compare(chunk.size() - 2, 2, "\r\n") == 0) {
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
            if ((chunk.size() >= _chunkSize + 2)) {
                _body += chunk.substr(0, _chunkSize);
                chunk = chunk.substr(_chunkSize, chunk.size());

                size_t index = chunk.find("\r\n");

                if (index != 0) {
                    if (!_reqLocation->root.empty())
                        findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                    else
                        findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                    Logger::warn("Chunked Body: not well formatted - missing CRLF");
                    return ;
                }

                if (_chunkSize == 0 && chunk == "\r\n") {
                    Logger::debug("Chunked Body: End found");
                    _chunkState = IS_END;
                    chunkRemaining = false;
                    if (isMultipart) {
                        _multipartState = GETTING_FIRST_BOUNDARY;
                        fullBody = _body;
                        chunk = _body;
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
                chunk = chunk.substr(index + 2, chunk.size());

                _chunkSize = -1;
                _chunkState = GETTING_SIZE;
                // std::cout << "Body is : ", printStr(_body), std::cout << " \nchunk i : ", printStr(_chunk), std::cout << std::endl; 
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
