#include "Request.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include "MimeTypes.hpp"
#include "colors.hpp"

void Request::checkMultipart(std::string content) {

    size_t index = content.find(";");
    std::string type, boundary;

    if (index == std::string::npos)
        return ;
    else { 
        type = content.substr(0, index);
        boundary = content.substr(index + 1);
        boundary = trimOws(boundary);

        index = boundary.find("=");
        if (index == 8) {
            std::string b = boundary.substr(0, 9);
            std::transform(b.begin(), b.end(), b.begin(), ::tolower);
            if (b != "boundary=") {
                return;
            }

            boundary = boundary.substr(9);
            if (boundary.empty() || boundary.size() > 70) {
                return;
            }

            if (boundary[0] == '\"' && boundary[boundary.size() - 1] == '\"') {
                boundary = boundary.substr(1, boundary.size() - 1);
            }

            if (!isMultipartCarac(boundary)) {
                return ;
            } else {
                _multipartBoundary = boundary;
            }
        } else {
            return ;
        }
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        if (type == "multipart/form-data")
            _isMultipart = true;
    }
}

bool Request::isMultipartCarac(std::string &boundary) {
    if (boundary.empty()) {
        return false;
    }

    for (std::string::size_type i = 0; i < boundary.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(boundary[i]);

        if (
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.'
        ) {
            continue;
        }

        return false;
    }

    return true;
}

// void printRNPositions(const std::string& str) {
//     std::cout << "\nENTERING RN POSITIONS" << std::endl;
//     for (size_t i = 0; i < str.size(); ++i) {
//         if (str[i] == '\r') {
//             std::cout << "\\r found at index " << i << std::endl;
//         } else if (str[i] == '\n') {
//             std::cout << "\\n found at index " << i << std::endl;
//         }
//     }
// }

bool Request::parseMultipart() {

    if (_multipartState == GETTING_FIRST_BOUNDARY) {
        size_t index = _chunk.find("--" + _multipartBoundary + "\r\n");

        if (index != 0 && _chunk.size() > _multipartBoundary.size() + 4 ) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "multipart: no first boundary" << std::endl;
            return false;

        } else if (_chunk.size() < _multipartBoundary.size() + 4) {
            _multipartRemaining = true;
            return true;

        } else {
            _chunk = _chunk.substr(index + _multipartBoundary.size() + 4);
            _multipartState = IS_HEADERS;
        }
    }

    
    while (_multipartState != IS_MULTI_END) {

        if (_reqLocation->bodySize < _fullBody.size()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(413, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(413, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error body is higher that client max body size for multipart" << std::endl;
            return false ;   
        }



        if (_multipartState == IS_HEADERS) {
            if (!handleMultipartHeader())
                return false;
        }
        if (_multipartState == IS_BODY) {

            std::string interBoundary = "\r\n--" + _multipartBoundary + "\r\n";

            size_t index = _chunk.find(interBoundary);
            if (index != std::string::npos) {
                std::string b = _chunk.substr(0, index);

                _chunk = _chunk.substr(index + interBoundary.size());
                _multiTemp.body = b;
                _multipartState = IS_HEADERS;
                _multipartContent.push_back(_multiTemp);
                _multiTemp.headers.clear(), _multiTemp.body.clear(), _multiTemp.name.clear(), _multiTemp.filename.clear();

                continue ;
            }

            std::string finalBoundary = "\r\n--" + _multipartBoundary + "--";
            size_t endIndex = _chunk.find(finalBoundary);
            if (endIndex != std::string::npos) {
                
                if (!checkLastBoundary(endIndex, finalBoundary))
                    return false;


                continue ;
            }
            
            _multipartRemaining = true;
            return true;
        }
    }
    return true;
}


bool Request::handleMultipartHeader() {
    
    size_t index = _chunk.find("\r\n");

    if (index == std::string::npos) {
        _multipartRemaining = true;
        return true;

    } else if (index == 0) {

        _chunk = _chunk.substr(index + 2);
        if (!checkMultipartHeader())
            return false;

    } else {
        std::string header = _chunk.substr(0, index);
        _chunk = _chunk.substr(index + 2);
        std::cout << "Getting header = " << header << "and remaining body is " << _chunk <<std::endl;

        size_t index = header.find(":");
        if (index == 0 || index == std::string::npos) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with multipart headers no : or no header name" << std::endl;
            return false;
        }
        std::string name = header.substr(0, index);
        std::cout << "HEADER NAME " << name << std::endl;
        if (hasWS(name)) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with multiparts headers name has WS" << std::endl;
            return false;
        }
        if (name != "Content-Disposition" && name != "Content-Type") {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with multiparts headers invalid" << std::endl;
            return false;
        }
        
        std::string content = header.substr(index + 1);
        
        if (!content.empty() && (content[0] != ' ' && content[0] != '\t')) {
            std::cout << "CONTENT = \'" << content << "\'" << std::endl;
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error with headers : is not followed by space" << std::endl;
            return false;
        }
        content = trimOws(content);
        std::cout << "HEADER CONTENT " << content << std::endl;
        if ((name == "Content-Disposition" && _multiTemp.headers.find("Content-Disposition") != _multiTemp.headers.end()) 
                || (name == "Content-Type" && _multiTemp.headers.find("Content-Type") != _multiTemp.headers.end())) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with duplicate multipart headers : " << name << std::endl;
            return false;  
        }
        if (((name == "Content-Disposition") && content.empty())
                || ((name == "Content-Type") && content.empty())) {
            findErrorPage(400, "/", _globalDir.errPage);
            std::cout << "error multipart header : " << name << " cannot have an empty content" << std::endl;
            return false;
        }

        _multiTemp.headers[name] = content;

    }
    return true;
}

bool Request::checkMultipartHeader() {

    std::map<std::string, std::string>::iterator it = _multiTemp.headers.find("Content-Disposition");

    if (it == _multiTemp.headers.end()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with multipart no Content Disposition" << std::endl;
        return false;

    } else {
        if(!checkContentDisposition(it->second))
            return false;

        it = _multiTemp.headers.find("Content-Type");

        if (it != _multiTemp.headers.end()) {
            std::string contentType = it->second;
            if (!checkContentType(it->second))
                return false;

        } else if (it == _multiTemp.headers.end() && _multiTemp.filename.empty()) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error with multipart  no content type and no filename" << std::endl;
                return false;

        } else if (it == _multiTemp.headers.end() && !_multiTemp.filename.empty()) {
                std::string type = MimeTypes::getType(_multiTemp.filename);
                if (type.empty()) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                    } else {
                        findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                    }
                    std::cout << "error with multipart  no content type and  filename has wring extension" << std::endl;
                    return false;
                }
        }

    }


    // std::cout << "\nMULTIPART HEADER MAP\n" << std::endl;
    // std::map<std::string, std::string>::iterator itmap = _multiTemp.headers.begin();
    // for (; itmap != _multiTemp.headers.end(); itmap++) {
    //     std::cout << "[" << itmap->first << ", " << itmap->second << "]" << std::endl;
    // }
    // std::cout << "NAME = " << _multiTemp.name << "and FILENAME = " << _multiTemp.filename << std::endl;

    _multipartState = IS_BODY;
    return true;
}


bool Request::checkContentDisposition(const std::string& content) {

    std::string contentDispo = content;

    size_t i = content.find("form-data");
    if (i != 0 || i == std::string::npos) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with multipart  Content Disposition does not start with form-data" << std::endl;
        return false;

    } else {
        contentDispo = contentDispo.substr(i + 9);
        contentDispo = trimOws(contentDispo);

        if (contentDispo[0] != ';') {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with multipart  Content Disposition wrong format after form data" << std::endl;
            return false;
        } else {
            contentDispo = contentDispo.substr(1);
        }
    }

    std::stringstream ss(contentDispo);
    std::string token;

    while (std::getline(ss, token, ';')) {

        token = trimOws(token);

        if (token.find("name=") == 0 && _multiTemp.name.empty()) {

            std::string val = token.substr(5);
            if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                _multiTemp.name = val.substr(1, val.size()-2);
            } else {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error with multipart  Content Disposition wrong format after name" << std::endl;
                return false;
            }

        } else if (token.find("filename=") == 0 && _multiTemp.filename.empty()) {

            std::string val = token.substr(9);
            if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                _multiTemp.filename = val.substr(1, val.size()-2);
            } else {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error with multipart  Content Disposition wrong format after filename" << std::endl;
                return false;
            }

        } else {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with multipart  Content Disposition wrong token" << std::endl;
            return false;
        }
    }
    return true;
}

bool Request::checkContentType(const std::string& content) {

    if (!MimeTypes::isSupportedType(content)) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(415, _reqLocation->root, _reqLocation->errPage);
        } else {
            findErrorPage(415, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with multipart  Content Type not supported" << std::endl;
        return false;

    } else if (!_multiTemp.filename.empty()) {

        std::string typeExt = MimeTypes::getType(_multiTemp.filename);

        if (typeExt != content) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with multipart  Content Type extension not same as filename" << std::endl;
            return false;
        }
    } 
    return true;
}

bool Request::checkLastBoundary(int endIndex, const std::string &finalBoundary) {

    std::string b = _chunk.substr(0, endIndex);

    _chunk = _chunk.substr(endIndex + finalBoundary.size());
    _chunk = trimFirstCRLF(_chunk);

    if (!_chunk.empty()) {
        findErrorPage(400, "/", _globalDir.errPage);
        std::cout << "error remaining body after end boundary" << std::endl;
        return false;
    }

    _multiTemp.body = b;
    _multipartState = IS_MULTI_END;
    _multipartRemaining = false;
    _multipartContent.push_back(_multiTemp);

    if (!_isChunked) {

        std::map<std::string, std::string>::iterator it = _headers.find("content-length");
        if (it == _headers.end()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(411, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(411, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error no content length but existing body for multipart" << std::endl;
            return false;
        } else {
            std::stringstream ss;
            ss << _fullBody.size(); // protect ?
            std::string bodySize = ss.str();
            if (it->second != bodySize) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error  content length is not equal to existing body size for multipart" << std::endl;
                return false;
            }
        }
    }

    return true;
}