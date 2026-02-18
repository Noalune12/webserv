#include <algorithm>
#include <iostream>
#include <sstream>

#include "Request.hpp"
#include "MimeTypes.hpp"
#include "Logger.hpp"

void Request::checkMultipart(const std::string& content) {

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
        if (type == "multipart/form-data") {
            Logger::debug("Body: is multipart");
            isMultipart = true;
        }
    }
}

bool Request::isMultipartCarac(const std::string &boundary) {
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

bool Request::parseMultipart() {

    if (_multipartState == GETTING_FIRST_BOUNDARY) {
        size_t index = chunk.find("--" + _multipartBoundary + "\r\n");

        if (index != 0 && chunk.size() > _multipartBoundary.size() + 4 ) {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: No first boundary");
            return false;

        } else if (chunk.size() < _multipartBoundary.size() + 4) {
            multipartRemaining = true;
            return true;

        } else {
            chunk = chunk.substr(index + _multipartBoundary.size() + 4);
            _multipartState = IS_HEADERS;
        }
    }


    while (_multipartState != IS_MULTI_END) {

        if (reqLocation->bodySize < fullBody.size()) {
            if (!reqLocation->root.empty()) {
                findErrorPage(413, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(413, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Body size higher than client max body size");
            return false ;
        }



        if (_multipartState == IS_HEADERS) {
            if (!handleMultipartHeader())
                return false;
        }
        if (_multipartState == IS_BODY) {

            std::string interBoundary = "\r\n--" + _multipartBoundary + "\r\n";

            size_t index = chunk.find(interBoundary);
            if (index != std::string::npos) {
                std::string b = chunk.substr(0, index);

                chunk = chunk.substr(index + interBoundary.size());
                _multiTemp.body = b;
                _multipartState = IS_HEADERS;
                _multipartContent.push_back(_multiTemp);
                _multiTemp.headers.clear(), _multiTemp.body.clear(), _multiTemp.name.clear(), _multiTemp.filename.clear();

                continue ;
            }

            std::string finalBoundary = "\r\n--" + _multipartBoundary + "--";
            size_t endIndex = chunk.find(finalBoundary);
            if (endIndex != std::string::npos) {

                if (!checkLastBoundary(endIndex, finalBoundary))
                    return false;


                continue ;
            }

            multipartRemaining = true;
            return true;
        }
    }
    return true;
}


bool Request::handleMultipartHeader() {

    size_t index = chunk.find("\r\n");

    if (index == std::string::npos) {
        multipartRemaining = true;
        return true;

    } else if (index == 0) {

        Logger::debug("Multipart: Headers end found");
        chunk = chunk.substr(index + 2);
        if (!checkMultipartHeader())
            return false;

    } else {
        std::string header = chunk.substr(0, index);
        chunk = chunk.substr(index + 2);

        size_t index = header.find(":");
        if (index == 0 || index == std::string::npos) {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Header not well defined");
            return false;
        }
        std::string name = header.substr(0, index);

        if (hasWS(name)) {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Header not well defined");
            return false;
        }
        if (name != "Content-Disposition" && name != "Content-Type") {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Header Name is invalid");
            return false;
        }

        std::string content = header.substr(index + 1);

        if (!content.empty() && (content[0] != ' ' && content[0] != '\t')) {

            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Header not well defined");
            return false;
        }
        content = trimOws(content);

        if ((name == "Content-Disposition" && _multiTemp.headers.find("Content-Disposition") != _multiTemp.headers.end())
                || (name == "Content-Type" && _multiTemp.headers.find("Content-Type") != _multiTemp.headers.end())) {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Header " + name + " is duplicated");
            return false;
        }
        if (((name == "Content-Disposition") && content.empty())
                || ((name == "Content-Type") && content.empty())) {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Header " + name + " has empty content");
            return false;
        }

        _multiTemp.headers[name] = content;

    }
    return true;
}

bool Request::checkMultipartHeader() {

    std::map<std::string, std::string>::iterator it = _multiTemp.headers.find("Content-Disposition");

    if (it == _multiTemp.headers.end()) {
        if (!reqLocation->root.empty()) {
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Multipart: Content Dispositon missing");
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
                if (!reqLocation->root.empty()) {
                    findErrorPage(400, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Multipart: Content Type and Filename missing");
                return false;

        } else if (it == _multiTemp.headers.end() && !_multiTemp.filename.empty()) {
                std::string type = MimeTypes::getType(_multiTemp.filename);
                if (type.empty()) {
                    if (!reqLocation->root.empty()) {
                        findErrorPage(400, reqLocation->root, reqLocation->errPage);
                    } else {
                        findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                    }
                    Logger::warn("Multipart: No Content Type and Filename has wrong extension");
                    return false;
                }
        }

    }

    Logger::debug("Multipart Headers");
    std::map<std::string, std::string>::iterator itmap = _multiTemp.headers.begin();
    for (; itmap != _multiTemp.headers.end(); itmap++) {
        std::cout << "[" << itmap->first << ", " << itmap->second << "]" << std::endl;
    }
    Logger::debug("Multipart: name = " + _multiTemp.name + " and filename = " + _multiTemp.filename);

    _multipartState = IS_BODY;
    return true;
}


bool Request::checkContentDisposition(const std::string& content) {

    std::string contentDispo = content;

    size_t i = content.find("form-data");
    if (i != 0 || i == std::string::npos) {
        if (!reqLocation->root.empty()) {
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Multipart: Content Disposition does not start with form-data");
        return false;

    } else {
        contentDispo = contentDispo.substr(i + 9);
        contentDispo = trimOws(contentDispo);

        if (contentDispo[0] != ';') {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Content Disposition not well defined after form-data");
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
                if (!reqLocation->root.empty()) {
                    findErrorPage(400, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Multipart: Content Disposition not well defined after name");
                return false;
            }

        } else if (token.find("filename=") == 0 && _multiTemp.filename.empty()) {

            std::string val = token.substr(9);
            if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
                _multiTemp.filename = val.substr(1, val.size()-2);
            } else {
                if (!reqLocation->root.empty()) {
                    findErrorPage(400, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Multipart: Content Disposition not well defined after filename");
                return false;
            }

        } else {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Content Disposition has wrong token");
            return false;
        }
    }
    return true;
}

bool Request::checkContentType(const std::string& content) {

    if (!MimeTypes::isSupportedType(content)) {
        if (!reqLocation->root.empty()) {
            findErrorPage(415, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(415, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Multipart: Content Type not supported");
        return false;

    } else if (!_multiTemp.filename.empty()) {

        std::string typeExt = MimeTypes::getType(_multiTemp.filename);

        if (typeExt != content) {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: Content Type and filename extension does not match");
            return false;
        }
    }
    return true;
}

bool Request::checkLastBoundary(int endIndex, const std::string &finalBoundary) {

    std::string b = chunk.substr(0, endIndex);

    chunk = chunk.substr(endIndex + finalBoundary.size());
    chunk = trimFirstCRLF(chunk);

    if (!chunk.empty()) {
        if (!reqLocation->root.empty()) {
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        } else {
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Multipart: Remaining body after final boundary");
        return false;
    }

    _multiTemp.body = b;
    _multipartState = IS_MULTI_END;
    multipartRemaining = false;
    _multipartContent.push_back(_multiTemp);

    if (!isChunked) {

        std::map<std::string, std::string>::iterator it = headers.find("content-length");
        if (it == headers.end()) {
            if (!reqLocation->root.empty()) {
                findErrorPage(411, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(411, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Multipart: No Content Lentgh");
            return false;
        } else {
            std::stringstream ss;
            ss << fullBody.size();
            std::string bodySize = ss.str();
            if (it->second != bodySize) {
                if (!reqLocation->root.empty()) {
                    findErrorPage(400, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(400, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Multipart: Content Lentgh and Body Size do not match");
                return false;
            }
        }
    }

    return true;
}
