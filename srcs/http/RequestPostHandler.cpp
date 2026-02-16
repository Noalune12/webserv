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

    Logger::debug("Entering POST Handler");

    if (fullBody.empty() && isMultipart == false) {
        if (!_reqLocation->root.empty())
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        Logger::warn("Post: Body is missing");
        return ;
    }

    if (!_trailing.empty()) {
        if (!_reqLocation->root.empty())
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);
        Logger::warn("Post: Trailing after location");
        return ;
    }

    if (isMultipart == true) {

        Logger::debug("Post: is multipart");

        if (!findUploadDir())
            return ;

        Logger::debug("Post: upload directory is " + _postUploadDir);

        handleMultipart();

        if (_totalUpload != 0 && _failedUpload == _totalUpload) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
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

        std::map<std::string, std::string>::iterator it = _headers.find("content-type");
        if (it == _headers.end()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            Logger::warn("Post: Content Type required");
            return ;
        } else {

            if (isMultipart == false &&  !MimeTypes::isSupportedType(it->second)) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(415, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(415, _reqLocation->alias, _reqLocation->errPage);
                }
                Logger::warn("Post: Content Type is not supportefd");
                return ;
            }

            _postExt = MimeTypes::getExtensionFromType(it->second);
            if (_postExt.empty()) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(415, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(415, _reqLocation->alias, _reqLocation->errPage);
                }
                Logger::warn("Post: Content Type is not supportefd");
                return ;
            }

        }

        if (!findUploadDir())
            return ;

        Logger::debug("Post: upload directory is " + _postUploadDir);
        
        std::string filename = "upload_";
        if (!createFileName(filename)) {
            if (!_reqLocation->root.empty()) { 
                findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
            }
            return;
        }

        Logger::debug("Post: creating " + _postFilename + " in directory " + _postUploadDir);

        std::ofstream outfile ((_postUploadDir + _postFilename).c_str());
        if(!outfile) {
            if (!_reqLocation->root.empty()) { 
                findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
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
        _uplaodFiles.push_back(temp);

    }
}

bool Request::findUploadDir() {

    if (_reqLocation->uploadTo.empty()) {
        if (!_reqLocation->root.empty())
            _postUploadDir = _reqLocation->root + _uri;
        else
            _postUploadDir = _reqLocation->alias;
    } else {
        _postUploadDir = _reqLocation->uploadTo;
    }
    if (_postUploadDir[0] == '/')
        _postUploadDir = _postUploadDir.substr(1, _postUploadDir.size());

    struct stat buf;
    if (stat(_postUploadDir.c_str(), &buf) != 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Post: upload dir does not exist");
        return false;
    }

    if (!S_ISDIR(buf.st_mode)) {
        if (!_reqLocation->root.empty())
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        Logger::warn("Post: upload dir is not a folder");
        return false;
    }

    if (access(_postUploadDir.c_str(), W_OK | X_OK) != 0) {
        if (!_reqLocation->root.empty())
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
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
                Logger::debug("Post: File with same name exists");
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


void Request::printFilename() const {
    std::vector<FilesPost>::const_iterator it = _uplaodFiles.begin();
    for (; it != _uplaodFiles.end(); it++) {
        Logger::debug("File was created with name : " + it->filename + " at location : " + it->location);
    }
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
                Logger::debug("Post: creating " + it->filename + " in directory " + _postUploadDir);

                outfile << it->body;

                outfile.close();

                FilesPost temp;
                temp.filename = it->filename;
                temp.location = _postUploadDir;
                _uplaodFiles.push_back(temp);
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
                Logger::debug("Post: creating " + _postFilename + " in directory " + _postUploadDir);

                outfile << it->body;

                outfile.close();
                FilesPost temp;
                temp.filename = _postFilename;
                temp.location = _postUploadDir;
                _uplaodFiles.push_back(temp);
                _totalUpload++;
            }

        }
    }
}