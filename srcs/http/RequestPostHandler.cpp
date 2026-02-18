#include "Request.hpp"
#include "MimeTypes.hpp"
#include "Logger.hpp"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <algorithm>


void Request::methodPostHandler() {


    if (fullBody.empty() && isMultipart == false) {
        if (!reqLocation->root.empty())
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        else
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        Logger::warn("Post: Body is missing");
        return ;
    }

    if (!trailing.empty()) {
        if (!reqLocation->root.empty())
            findErrorPage(404, reqLocation->root, reqLocation->errPage);
        else
            findErrorPage(404, reqLocation->alias, reqLocation->errPage);
        Logger::warn("Post: Trailing after location");
        return ;
    }

    if (isMultipart == true) {

        if (!findUploadDir())
            return ;

        handleMultipart();

        if (_totalUpload != 0 && _failedUpload == _totalUpload) {
            if (!reqLocation->root.empty()) {
                findErrorPage(500, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(500, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Post: All uploads failed");
            return ;
        } else if (_failedUpload == 0 && _totalUpload == 0) {
            err = false;
            status = 200;
        } else {
            err = false;
            status = 201;
        }

    } else {

        std::map<std::string, std::string>::iterator it = headers.find("content-type");
        if (it == headers.end()) {
            if (!reqLocation->root.empty()) {
                findErrorPage(400, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(400, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Post: Content Type required");
            return ;
        } else {

            if (isMultipart == false &&  !MimeTypes::isSupportedType(it->second)) {
                if (!reqLocation->root.empty()) {
                    findErrorPage(415, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(415, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Post: Content Type is not supportefd");
                return ;
            }

            _postExt = MimeTypes::getExtensionFromType(it->second);
            if (_postExt.empty()) {
                if (!reqLocation->root.empty()) {
                    findErrorPage(415, reqLocation->root, reqLocation->errPage);
                } else {
                    findErrorPage(415, reqLocation->alias, reqLocation->errPage);
                }
                Logger::warn("Post: Content Type is not supportefd");
                return ;
            }

        }

        if (!findUploadDir())
            return ;
        
        std::string filename = "upload_";
        if (!createFileName(filename)) {
            if (!reqLocation->root.empty()) { 
                findErrorPage(500, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(500, reqLocation->alias, reqLocation->errPage);
            }
            return;
        }

        std::ofstream outfile ((_postUploadDir + _postFilename).c_str());
        if(!outfile) {
            if (!reqLocation->root.empty()) { 
                findErrorPage(500, reqLocation->root, reqLocation->errPage);
            } else {
                findErrorPage(500, reqLocation->alias, reqLocation->errPage);
            }
            Logger::warn("Post: outfile can not be created");
            return;
        }

        outfile << fullBody;

        outfile.close();

        err = false;
        status = 201;
        FilesPost temp;
        temp.filename = _postFilename;
        temp.location = _postUploadDir;
        uploadFiles.push_back(temp);

    }
}

bool Request::findUploadDir() {

    if (reqLocation->uploadTo.empty()) {
        if (!reqLocation->root.empty())
            _postUploadDir = reqLocation->root + uri;
        else
            _postUploadDir = reqLocation->alias;
    } else {
        _postUploadDir = reqLocation->uploadTo;
    }
    if (_postUploadDir[0] == '/')
        _postUploadDir = _postUploadDir.substr(1, _postUploadDir.size());

    struct stat buf;
    if (stat(_postUploadDir.c_str(), &buf) != 0) {
        if (!reqLocation->root.empty()) {
            findErrorPage(404, reqLocation->root, reqLocation->errPage);
        }
        else {
            findErrorPage(404, reqLocation->alias, reqLocation->errPage);
        }
        Logger::warn("Post: upload dir does not exist");
        return false;
    }

    if (!S_ISDIR(buf.st_mode)) {
        if (!reqLocation->root.empty())
            findErrorPage(400, reqLocation->root, reqLocation->errPage);
        else
            findErrorPage(400, reqLocation->alias, reqLocation->errPage);
        Logger::warn("Post: upload dir is not a folder");
        return false;
    }

    if (access(_postUploadDir.c_str(), W_OK | X_OK) != 0) {
        if (!reqLocation->root.empty())
            findErrorPage(403, reqLocation->root, reqLocation->errPage);
        else
            findErrorPage(403, reqLocation->alias, reqLocation->errPage);
        Logger::warn("Post: no rights on upload dir");
        return false;
    }

    if (_postUploadDir[_postUploadDir.size() - 1] != '/') {
        _postUploadDir = _postUploadDir + "/";
    }

    return true;
}

bool Request::checkFilename(const std::string &filename) {
    DIR* dir = opendir(_postUploadDir.c_str());
    if (dir == NULL) {
        Logger::warn("Post: opendir failed");
        return false;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        std::string n = entry->d_name;
        if (n == "." || n == "..")
            continue;

        struct stat st;

        if (stat((_postUploadDir + n).c_str(), &st) == 0) {

            if (!S_ISREG(st.st_mode))
                continue;
            if (n == filename) {
                closedir(dir);
                return false;
            }

        } else {
            closedir(dir);
            return false;
        }

    }

    closedir(dir);
    return true;
}


bool Request::createFileName(const std::string &filename) {
    double add = 1;

    DIR* dir = opendir(_postUploadDir.c_str()); 
    if (dir == NULL) {
        Logger::warn("Post: opendir failed");
        return false;
    }

    struct dirent* entry;

    std::vector<std::string> filesExt;

    while ((entry = readdir(dir)) != NULL) {
        std::string n = entry->d_name;
        if (n == "." || n == "..")
            continue;

        struct stat st;

        if (stat((_postUploadDir + n).c_str(), &st) == 0) {

            if (!S_ISREG(st.st_mode))
                continue;

            size_t index = n.find(filename);
            if (index == std::string::npos || index != 0)
                continue;
    
            std::string temp = n.substr(filename.size());
            
            index = temp.find(".");
            if (index != std::string::npos) {
                temp = temp.substr(0, index);
            }

            filesExt.push_back(temp);

        } else {
            closedir(dir);
            return false;
        }

    }
    closedir(dir);

    std::sort(filesExt.begin(), filesExt.end());

    std::vector<std::string>::iterator index;
    std::stringstream ss;
    ss << add;
    if (ss.fail()) {
        Logger::warn("Post: extension conversion failed");
        return false;
    }
    std::string addStr = ss.str();

    while ((index = std::find(filesExt.begin(), filesExt.end(), addStr)) != filesExt.end()) {
        std::stringstream sss;
        add++;
        if (add > __DBL_MAX__) {
            Logger::warn("Post: too many files with the same name");
            return false;
        }
        sss << add;
        if (sss.fail()) {
            Logger::warn("Post: extension conversion failed");
            return false;
        }
        addStr = sss.str();
    }

    _postFilename = filename + addStr + _postExt;
    return true;
}

void Request::handleMultipart() {

    std::vector<Multipart>::iterator it = _multipartContent.begin();

    for (; it != _multipartContent.end(); it++) {

        if (it->name == "file") {

            if (!it->filename.empty() && checkFilename(it->filename)) {
                std::ofstream outfile ((_postUploadDir + it->filename).c_str());
                if (!outfile) {
                    _failedUpload++, _totalUpload++;
                    continue;
                }

                outfile << it->body;

                outfile.close();

                FilesPost temp;
                temp.filename = it->filename;
                temp.location = _postUploadDir;
                uploadFiles.push_back(temp);
                _totalUpload++;

            } else {

                if (!it->filename.empty()) {
                    size_t index = it->filename.find(".");
                    std::string filename = it->filename;
                    if (index != std::string::npos) {
                        filename = filename.substr(0, index);
                        _postExt = it->filename.substr(index);
                    }
                    if (!createFileName(filename + "_")) {
                        _failedUpload++, _totalUpload++;
                        continue;
                    }

                } else {

                    std::map<std::string, std::string>::iterator itType = it->headers.find("Content-Type");
                    if (itType != it->headers.end()) {
                        _postExt = MimeTypes::getExtensionFromType(itType->second);

                    if (_postExt.empty()) {
                        _failedUpload++, _totalUpload++;
                        continue;
                    }

                    std::string filename = "upload_";
                    if (!createFileName(filename)) {
                        _failedUpload++, _totalUpload++;
                        continue;
                    }

                    } else {
                        _failedUpload++, _totalUpload++;
                        continue;
                    }
                }

                std::ofstream outfile ((_postUploadDir + _postFilename).c_str());
                if (!outfile) {
                    _failedUpload++, _totalUpload++;
                    continue;
                }

                outfile << it->body;

                outfile.close();
                FilesPost temp;
                temp.filename = _postFilename;
                temp.location = _postUploadDir;
                uploadFiles.push_back(temp);
                _totalUpload++;
            }

        }
    }
}
