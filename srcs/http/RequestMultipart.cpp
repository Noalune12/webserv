#include "Request.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include "MimeTypes.hpp"
#include "colors.hpp"

void Request::checkMultipart(std::string content) {
    std::cout << "\nCHECK MULTIPART : " << content << std::endl;
    size_t index = content.find(";");
    std::string type, boundary;
    if (index == std::string::npos)
        return ;
    else { 
        type = content.substr(0, index);
        boundary = content.substr(index + 1);
        boundary = trimOws(boundary);
        std::cout << type << " with boundary \'" << boundary << "\'" << std::endl;
        index = boundary.find("=");
        if (index == 8) {
            std::string b = boundary.substr(0, 9);
            std::transform(b.begin(), b.end(), b.begin(), ::tolower);
            std::cout << "B = " << b << std::endl;
            if (b != "boundary=") {
                // eroor ?
                return;
            }
            boundary = boundary.substr(9);
            std::cout << "BOUNDARY IS = " << boundary << std::endl;
            if (boundary.empty() || boundary.size() > 70) {
                // error ?
                return;
            }
            if (boundary[0] == '\"' && boundary[boundary.size() - 1] == '\"') {
                boundary = boundary.substr(1, boundary.size() - 1); // to test
            }
            if (!isMultipartCarac(boundary)) {
                //error ?
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

void printRNPositions(const std::string& str) {
    std::cout << "\nENTERING RN POSITIONS" << std::endl;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\r') {
            std::cout << "\\r found at index " << i << std::endl;
        } else if (str[i] == '\n') {
            std::cout << "\\n found at index " << i << std::endl;
        }
    }
}

bool Request::parseMultipart() {

    // _fullBody += _chunk;
    std::cout << RED "\nENTERING MULTIPART\n\n" RESET;
    
    // std::cout << "CHUNK = " << _chunk << std::endl << std::endl;
    // std::string finalBoundary = "\r\n--" + _multipartBoundary + "--";
    // size_t endIndex = _chunk.find(finalBoundary);
    // if (endIndex != std::string::npos) {
    //     std::string b = _chunk.substr(0, endIndex);
    //     _chunk = _chunk.substr(endIndex + finalBoundary.size());
    //     // printRNPositions(_chunk);
    //     _chunk = trimFirstCRLF(_chunk);
    //     std::cout << "B IS \'" << b << "\'" << std::endl; 
    //     std::cout << "REMAINING BODY IS \'" << _chunk << "\'" << std::endl; 
    //     if (!_chunk.empty()) {
    //         findErrorPage(400, "/", _globalDir.errPage);
    //         std::cout << "error remaining body after end boundary" << std::endl;
    //         return false;
    //     }

    //     _multiTemp.body = b;
    //     _multipartState = IS_MULTI_END;
    //     _multipartRemaining = false;
    //     _multipartContent.push_back(_multiTemp);
    //     return true ;
    // } else {
    //     _multipartRemaining = true;
    // }
    // check if full body > client max body size + if content len exist compare (if not chunked)
    // printRNPositions(_chunk);
    if (_multipartState == GETTING_FIRST_BOUNDARY) {
        size_t index = _chunk.find("--" + _multipartBoundary + "\r\n");
        if (index != 0 && _chunk.size() > _multipartBoundary.size() + 4 ) {
            // error
            std::cout << "multipart: no first boundary" << std::endl; // to test
            return false;
        } else if (_chunk.size() < _multipartBoundary.size() + 4) {
            _multipartRemaining = true;
            return true;
        } else {
            _chunk = _chunk.substr(index + _multipartBoundary.size() + 4);
            std::cout << "body is after 1st boundary = " << _chunk << std::endl;
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
            // case sensitive, only Content-Disposition and Content-Type -> 400 invalid multipart header or ignore
            size_t index = _chunk.find("\r\n");
            if (index == std::string::npos) {
                std::cout << "waiting for headers" << std::endl;
                _multipartRemaining = true;
                return true;
            } else if (index == 0) {
                std::cout << "end of headers" << std::endl;
                _chunk = _chunk.substr(index + 2);
                // check if Content-Disposition is here and if it is well formated + si content-type correspond a l'extension

                std::map<std::string, std::string>::iterator it = _multiTemp.headers.find("Content-Disposition");

                if (it == _multiTemp.headers.end()) {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
                    std::cout << "error with multipart no Content Disposition" << std::endl;
                    return false;
                } else {
                    std::string contentDispo = it->second;
                    size_t i = it->second.find("form-data");
                    if (i != 0 || i == std::string::npos) {
                        findErrorPage(400, "/", _globalDir.errPage); // host identified
                        std::cout << "error with multipart  Content Disposition does not start with form-data" << std::endl;
                        return false;
                    } else {
                        contentDispo = contentDispo.substr(i + 9);
                        contentDispo = trimOws(contentDispo);
                        if (contentDispo[0] != ';') {
                            findErrorPage(400, "/", _globalDir.errPage); // host identified
                            std::cout << "error with multipart  Content Disposition wrong format after form data" << std::endl;
                            return false;
                        } else {
                            contentDispo = contentDispo.substr(1);
                        }
                    }
                    std::stringstream ss(contentDispo);
                    std::string token;
                    std::cout << "TOKENS ;" << std::endl;
                    while (std::getline(ss, token, ';')) {
                        std::cout << token << std::endl;
                        token = trimOws(token);
                        if (token.find("name=") == 0 && _multiTemp.name.empty()) {
                            std::string val = token.substr(5);
                            if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                                _multiTemp.name = val.substr(1, val.size()-2);
                            } else {
                                findErrorPage(400, "/", _globalDir.errPage); // host identified
                                std::cout << "error with multipart  Content Disposition wrong format after name" << std::endl;
                                return false;
                            }
                        }
                        else if (token.find("filename=") == 0 && _multiTemp.filename.empty()) {
                            std::string val = token.substr(9);
                            if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                                _multiTemp.filename = val.substr(1, val.size()-2);
                            } else {
                                findErrorPage(400, "/", _globalDir.errPage); // host identified
                                std::cout << "error with multipart  Content Disposition wrong format after filename" << std::endl;
                                return false;
                            }
                        } else {
                            findErrorPage(400, "/", _globalDir.errPage); // host identified
                            std::cout << "error with multipart  Content Disposition wrong token" << std::endl;
                            return false;
                        }
                    }

                    it = _multiTemp.headers.find("Content-Type");
                    if (it != _multiTemp.headers.end()) {
                        std::string contentType = it->second;
                        if (!MimeTypes::isSupportedType(it->second)) {
                            findErrorPage(415, "/", _globalDir.errPage); // host identified
                            std::cout << "error with multipart  Content Type not supported" << std::endl;
                            return false;
                        } else if (!_multiTemp.filename.empty()) {
                            // std::string filenameExt = MimeTypes::getExtension(_multiTemp.filename);
                            // size_t		lastSlash = it->second.find_last_of('/');
                            std::string typeExt = MimeTypes::getType(_multiTemp.filename);
                            // if (lastSlash != std::string::npos) {
                            //     typeExt += it->second.substr(lastSlash + 1);
                            // }
                            std::cout << "type Ext = " << typeExt << std::endl;
                            if (typeExt != it->second) {
                                findErrorPage(400, "/", _globalDir.errPage); // host identified
                                std::cout << "error with multipart  Content Type extension not same as filename" << std::endl;
                                return false;
                            }
                        } 
                    } else if (it == _multiTemp.headers.end() && _multiTemp.filename.empty()) { // no content type and no filename --> 400
                            findErrorPage(400, "/", _globalDir.errPage); // host identified
                            std::cout << "error with multipart  no content type and no filename" << std::endl;
                            return false;
                    } else if (it == _multiTemp.headers.end() && !_multiTemp.filename.empty()) { // no content type and  filename --> check type of extansion
                            std::string type = MimeTypes::getType(_multiTemp.filename);
                            if (type.empty()) {
                                findErrorPage(400, "/", _globalDir.errPage); // host identified
                                std::cout << "error with multipart  no content type and  filename has wring extension" << std::endl;
                                return false;
                            }
                    }

                }


                std::cout << "\nMULTIPART HEADER MAP\n" << std::endl;
                std::map<std::string, std::string>::iterator itmap = _multiTemp.headers.begin();
                for (; itmap != _multiTemp.headers.end(); itmap++) {
                    std::cout << "[" << itmap->first << ", " << itmap->second << "]" << std::endl;
                }
                std::cout << "NAME = " << _multiTemp.name << "and FILENAME = " << _multiTemp.filename << std::endl;
                _multipartState = IS_BODY;
            } else {
                std::string header = _chunk.substr(0, index);
                _chunk = _chunk.substr(index + 2);
                std::cout << "Getting header = " << header << "and remaining body is " << _chunk <<std::endl;

                size_t index = header.find(":");
                if (index == 0 || index == std::string::npos) {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
                    std::cout << "error with multipart headers no : or no header name" << std::endl;
                    return false;
                }
                std::string name = header.substr(0, index);
                std::cout << "HEADER NAME " << name << std::endl;
                if (hasWS(name)) {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
                    std::cout << "error with multiparts headers name has WS" << std::endl;
                    return false;
                }
                if (name != "Content-Disposition" && name != "Content-Type") {
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
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
                    findErrorPage(400, "/", _globalDir.errPage); // host identified
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
        }
        if (_multipartState == IS_BODY) {
            std::cout  << "IS BODY = " << _chunk << std::endl;
            std::string interBoundary = "\r\n--" + _multipartBoundary + "\r\n";
            size_t index = _chunk.find(interBoundary);
            if (index != std::string::npos) {
                std::string b = _chunk.substr(0, index);
                std::cout << "BODY B IS = " << b << std::endl;
                _chunk = _chunk.substr(index + interBoundary.size());
                _multiTemp.body = b;
                _multipartState = IS_HEADERS;
                _multipartContent.push_back(_multiTemp);
                _multiTemp.headers.clear();
                _multiTemp.body.clear();
                _multiTemp.name.clear();
                _multiTemp.filename.clear();
                continue ;
            }
            std::string finalBoundary = "\r\n--" + _multipartBoundary + "--";
            size_t endIndex = _chunk.find(finalBoundary);
            if (endIndex != std::string::npos) {
                std::string b = _chunk.substr(0, endIndex);
                _chunk = _chunk.substr(endIndex + finalBoundary.size());
                // printRNPositions(_chunk);
                _chunk = trimFirstCRLF(_chunk);
                std::cout << "B IS \'" << b << "\'" << std::endl; 
                std::cout << "REMAINING BODY IS \'" << _chunk << "\'" << std::endl; 
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
                        std::cout << "CONTENT LENGTH = " << it->second << "vs full body size = " << _fullBody.size() << std::endl;
                        std::stringstream ss;
                        ss << _fullBody.size();
                        std::string bodySize = ss.str();
                        std::cout << "BODY SIZE = " << bodySize << std::endl;
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


                continue ;
            }
            std::cout << "waiting for final or intermediate boundary" << std::endl;
            _multipartRemaining = true;
            return true;
        }
    }
    return true;
}
