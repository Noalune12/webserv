#include "Request.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include "MimeTypes.hpp"

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

    // check if full body > client max body size + if content len exist compare (if not chunked)
    printRNPositions(_body);
    if (_multipartState == GETTING_FIRST_BOUNDARY) {
        size_t index = _body.find("--" + _multipartBoundary + "\r\n");
        if (index != 0 && _body.size() > _multipartBoundary.size() + 4 ) {
            // error
            std::cout << "multipart: no first boundary" << std::endl; // to test
            return false;
        } else {
            _body = _body.substr(index + _multipartBoundary.size() + 4);
            std::cout << "body is after 1st boundary = " << _body << std::endl;
            _multipartState = IS_HEADERS;
        }
    }

    Multipart multi;
    while (_multipartState != IS_MULTI_END) {
        if (_multipartState == IS_HEADERS) {
            // case sensitive, only Content-Disposition and Content-Type -> 400 invalid multipart header or ignore
            size_t index = _body.find("\r\n");
            if (index == std::string::npos) {
                std::cout << "waiting for headers" << std::endl;
                _multipartRemaining = true;
                return true;
            } else if (index == 0) {
                std::cout << "end of headers" << std::endl;
                _body = _body.substr(index + 2);
                // check if Content-Disposition is here and if it is well formated + si content-type correspond a l'extension

                std::map<std::string, std::string>::iterator it = multi.headers.find("Content-Disposition");

                if (it == multi.headers.end()) {
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
                        if (token.find("name=") == 0 && multi.name.empty()) {
                            std::string val = token.substr(5);
                            if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                                multi.name = val.substr(1, val.size()-2);
                            } else {
                                findErrorPage(400, "/", _globalDir.errPage); // host identified
                                std::cout << "error with multipart  Content Disposition wrong format after name" << std::endl;
                                return false;
                            }
                        }
                        else if (token.find("filename=") == 0 && multi.filename.empty()) {
                            std::string val = token.substr(9);
                            if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                                multi.filename = val.substr(1, val.size()-2);
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

                    it = multi.headers.find("Content-Type");
                    if (it != multi.headers.end()) {
                        std::string contentType = it->second;
                        if (!MimeTypes::isSupportedType(it->second)) {
                            findErrorPage(400, "/", _globalDir.errPage); // host identified
                            std::cout << "error with multipart  Content Type not supported" << std::endl;
                            return false;
                        } else if (!multi.filename.empty()) {
                            std::string filenameExt = MimeTypes::getExtension(multi.filename);
                            size_t		lastSlash = it->second.find_last_of('/');
                            std::string typeExt = ".";
                            if (lastSlash != std::string::npos) {
                                typeExt += it->second.substr(lastSlash + 1);
                            }
                            if (typeExt != filenameExt) {
                                findErrorPage(400, "/", _globalDir.errPage); // host identified
                                std::cout << "error with multipart  Content Type extension not same as filename" << std::endl;
                                return false;
                            }
                        }
                    }

                }


                std::cout << "\nMULTIPART HEADER MAP\n" << std::endl;
                std::map<std::string, std::string>::iterator itmap = multi.headers.begin();
                for (; itmap != multi.headers.end(); itmap++) {
                    std::cout << "[" << itmap->first << ", " << itmap->second << "]" << std::endl;
                }
                std::cout << "NAME = " << multi.name << "and FILENAME = " << multi.filename << std::endl;
                _multipartState = IS_BODY;
            } else {
                std::string header = _body.substr(0, index);
                _body = _body.substr(index + 2);
                std::cout << "Getting header = " << header << "and remaining body is " << _body <<std::endl;

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
                if ((name == "Content-Disposition" && multi.headers.find("Content-Disposition") != multi.headers.end()) 
                        || (name == "Content-Type" && multi.headers.find("Content-Type") != multi.headers.end())) {
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

                multi.headers[name] = content;

            }
        }
        if (_multipartState == IS_BODY) {
            std::cout  << "IS BODY = " << _body << std::endl;
            std::string interBoundary = "\r\n--" + _multipartBoundary + "\r\n";
            size_t index = _body.find(interBoundary);
            if (index != std::string::npos) {
                std::string b = _body.substr(0, index);
                std::cout << "BODY B IS = " << b << std::endl;
                _body = _body.substr(index + interBoundary.size());
                multi.body = b;
                _multipartState = IS_HEADERS;
                _multipartContent.push_back(multi);
                multi.headers.clear();
                multi.body.clear();
                multi.name.clear();
                multi.filename.clear();
                continue ;
            }
            std::string finalBoundary = "\r\n--" + _multipartBoundary + "--";
            size_t endIndex = _body.find(finalBoundary);
            if (endIndex != std::string::npos) {
                std::string b = _body.substr(0, endIndex);
                _body = _body.substr(endIndex + finalBoundary.size());
                // printRNPositions(_body);
                _body = trimFirstCRLF(_body);
                std::cout << "B IS \'" << b << "\'" << std::endl; 
                std::cout << "REMAINING BODY IS \'" << _body << "\'" << std::endl; 
                if (!_body.empty()) {
                    findErrorPage(400, "/", _globalDir.errPage);
                    std::cout << "error remaining body after end boundary" << std::endl;
                    return false;
                }

                multi.body = b;
                _multipartState = IS_MULTI_END;
                _multipartRemaining = false;
                _multipartContent.push_back(multi);
                continue ;
            }
            std::cout << "waiting for final or intermediate boundary" << std::endl;
            _multipartRemaining = true;
            break;
        }
    }
    return true;
}
