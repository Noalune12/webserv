#include "Request.hpp"
#include <iostream>
#include "MimeTypes.hpp"
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <algorithm>


void Request::methodPostHandler() {

    if (_body.empty() && _isMultipart == false) {
        if (!_reqLocation->root.empty())
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, no body" << std::endl;
        return ;
    }

    if (!_trailing.empty()) {
        if (!_reqLocation->root.empty())
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, trailing after location not handled --> NOT SURE" << std::endl;
        return ;
    }

    if (_isMultipart == true) {

        if (!findUploadDir())
            return ;

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
                        if (!createFileName(filename)) {
                            _failedUpload++, _totalUpload++;
                            continue;
                        }
                    } else {
                        std::map<std::string, std::string>::iterator itType = it->headers.find("Content-Type");
                        if (itType != it->headers.end()) {
                            _postExt = MimeTypes::getExtensionFromType(itType->second);
                        if(_postExt.empty()) {
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
                    _uplaodFiles.push_back(temp);
                    _totalUpload++;
                }

            }
        }
        if (_totalUpload != 0 && _failedUpload == _totalUpload) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error nothing uploaded for multipart" << std::endl;
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
            std::cout << "error content type required for POST" << std::endl;
            return ;
        } else {

            if (_isMultipart == false &&  !MimeTypes::isSupportedType(it->second)) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(415, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(415, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error type is not supported" << std::endl;
                return ;
            }

            _postExt = MimeTypes::getExtensionFromType(it->second);
            if (_postExt.empty()) {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(415, _reqLocation->root, _reqLocation->errPage);
                } else {
                    findErrorPage(415, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error type is not supported" << std::endl;
                return ;
            }

        }

        if (!findUploadDir())
            return ;

        std::string filename = "upload_";
        if (!createFileName(filename)) {
            if (!_reqLocation->root.empty()) { 
                findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
            }
            return;
        }


        // std::cout << "FILENAME = " << _postFilename << "  in directory = " << _postUploadDir << std::endl;


        std::ofstream outfile ((_postUploadDir + _postFilename).c_str());
        if(!outfile) {
            if (!_reqLocation->root.empty()) { 
                findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
            } else {
                findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
            }
            return;
        }

        outfile << _fullBody;

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
        std::cout << "error with POST folder does not exist" << std::endl;
        return false;
    }

    if (!S_ISDIR(buf.st_mode)) {
        if (!_reqLocation->root.empty())
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, upload to is not a folder" << std::endl;
        return false;
    }

    if (access(_postUploadDir.c_str(), W_OK | X_OK) != 0) {
        if (!_reqLocation->root.empty())
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, no right on folder" << std::endl;
        return false;
    }

    if (_postUploadDir[_postUploadDir.size() - 1] != '/') {
        _postUploadDir = _postUploadDir + "/";
    }

    return true;
}

bool Request::checkFilename(std::string &filename) {
    DIR* dir = opendir(_postUploadDir.c_str());
    if (dir == NULL) {
        std::cout << "DELETE open dir fail" << std::endl;
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


bool Request::createFileName(std::string &filename) {
    double add = 1;

    DIR* dir = opendir(_postUploadDir.c_str()); 
    if (dir == NULL) {
        std::cout << "DELETE open dir fail" << std::endl;
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
        std::cout << "error  while converting extension for POST" << std::endl;
        return false;
    }
    std::string addStr = ss.str();

    while ((index = std::find(filesExt.begin(), filesExt.end(), addStr)) != filesExt.end()) {
        std::stringstream sss;
        add++;
        if (add > __DBL_MAX__) {
            std::cout << "Cannot upload more files" << std::endl;
            return false;
        }
        sss << add;
        if (sss.fail()) {
            std::cout << "error  while converting extension for POST" << std::endl;
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
        std::cout << "File was created with name : " << it->filename << " at location : " << it->location << std::endl;
    }
}
