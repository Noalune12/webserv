#include "Request.hpp"
#include "iostream"
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include "colors.hpp"
#include "unistd.h"
void Request::methodGetHandler() {

    if (!_reqLocation->root.empty() && !_trailing.empty()) {
        _getPath = getPath(_reqLocation->root + _uri, _trailing);
    } else if (!_reqLocation->root.empty() && _trailing.empty()) {
        _getPath = getPath(_reqLocation->root + _uri);
    } else if (!_reqLocation->alias.empty() && !_trailing.empty()) {
        _getPath = getPath(_reqLocation->alias, _trailing);
    } else if (!_reqLocation->alias.empty() && _trailing.empty()) {
        _getPath = getPath(_reqLocation->alias);
    }

    std::vector<std::string>::iterator itIndex = _reqLocation->index.begin();

    struct stat buf;
    if (stat(_getPath.c_str(), &buf) == 0) {
        if (_trailing.empty()) {

            // Case 1: no trailing and path is a folder -> need to get the index

            if (S_ISDIR(buf.st_mode)) {
                if (access(_getPath.c_str(), R_OK | X_OK) != 0) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    std::cout << "error with GET (not trailing) -> no right on folder" << std::endl;
                    return ;
                }

                for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                    std::string path = getPath(_getPath, *itIndex);

                    struct stat st;

                    if (stat(path.c_str(), &st) == 0) {
                        if (readFile(path, st))
                            return ;
                    }
                }

                if (err == true && _indexFound == true)
                    return;
                else if (_reqLocation->autoIndex == true) {
                    if (!handleAutoindex(_getPath))
                        return ;
                    err = false;
                    status = 200;
                    return;
                } else {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    std::cout << "error with GET no file found" << std::endl;
                    return ;
                }

            } else {
                if (!_reqLocation->root.empty()) {
                    findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                }
                else {
                    findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                }
                std::cout << "error with GET not a directory set as the path looking for index" << std::endl;
                return ;
            }
        } else {

            // Case 2: trailing and path is a folder -> need to get the file mentionned in trailing

            if (S_ISDIR(buf.st_mode)) {
                if (access(_getPath.c_str(), R_OK | X_OK) != 0) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    std::cout << "error with GET (trailing) -> no right on folder" << std::endl;
                    return ;
                }

                for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                    std::string path = getPath(_getPath, *itIndex);

                    struct stat st;

                    if (stat(path.c_str(), &st) == 0) {
                        if (readFile(path, st))
                            return ;
                    }
                }

                if (err == true && _indexFound == true)
                    return;
                else if (_reqLocation->autoIndex == true) {
                    if (!handleAutoindex(_getPath))
                        return ;
                    err = false;
                    status = 200;
                    return;
                } else {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    std::cout << "error with GET no file found" << std::endl;
                    return ;
                }

            // Case 3: trailing and path is a file -> need to get the file

            } else if (S_ISREG(buf.st_mode)) {

                readFile(_getPath, buf);
                return ;

            }
        }

    } else if (_trailing.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with GET no trailing, non existing path" << std::endl;
        return ;

    } else if (!_trailing.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with GET trailing, non existing path" << std::endl;
        return ;
    }

}

std::string Request::getPath(std::string folder) {

    std::string path;

    if (folder[folder.size() - 1] != '/' && !_trailing.empty())
        path = folder + "/" + _trailing;
    else
        path = folder + _trailing;

    if (path[0] == '/')
        path = path.substr(1, path.size());

    return path;
}

std::string Request::getPath(std::string folder, std::string file) {

    std::string path;

    if (folder[folder.size() - 1] == '/' && file[0] == '/')
        file = file.substr(1);
    if (folder[folder.size() - 1] == '/')
        path = folder + file;
    else
        path = folder + "/" + file;

    if (path[0] == '/')
        path = path.substr(1, path.size());

    return path;
}

bool Request::readFile(std::string path, struct stat buf) {


    if (!S_ISREG(buf.st_mode)) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error file is not regular" << std::endl;
        return false;
    }

    if (!access(path.c_str(), R_OK | X_OK)) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error file no rights" << std::endl;
        return false;
    }

    if (buf.st_mode & S_IRUSR) {
        std::ifstream file(path.c_str());
        std::cout << "File found" << std::endl;
        std::stringstream buffer;
        buffer << file.rdbuf(); // protect ?
        htmlPage = buffer.str();
        err = false;
        status = 200;
        return true;
    } else {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error file found but no rights" << std::endl;
        _indexFound = true;
        return false;
    }
}

bool Request::handleAutoindex(std::string dirPath) {

    if (access(dirPath.c_str(), R_OK | X_OK) != 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with GET AUTOINDEX no rights on a directory" << std::endl;
        return false;
    }


    std::string path = _reqLocation->path + _trailing;
    if (path[path.size() - 1] == '/')
        path = path.substr(0, path.size() - 1);
    htmlPage = "<!DOCTYPE html>\n<html>\n<head>\n <meta charset=\"UTF-8\">\n<title>Index</title>\n</head><h1>Index of " + path + "\n\n</h1>";


    DIR* dir = opendir(dirPath.c_str());
    if (dir == NULL) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with GET AUTOINDEX opendir failed" << std::endl;
        return false;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {

        std::string name = entry->d_name;

        if (name == "." || name == "..")
            continue;

        std::string fullPath;

        if (dirPath[dirPath.size() - 1] != '/')
            fullPath = dirPath + "/" + name;
        else
            fullPath = dirPath + name;
        
        struct stat st;

        if (stat(fullPath.c_str(), &st) == 0 ) {

            if (S_ISDIR(st.st_mode) && access(fullPath.c_str(), R_OK | X_OK) == 0) {
    
                htmlPage += "<a href=\"" + name + "/\">📁 " + name + "</a><br>\n";
    
            } else if (S_ISREG(st.st_mode) && access(fullPath.c_str(), R_OK) == 0) {
    
                htmlPage += "<a href=\"" + name + "\">📄 " + name + "</a><br>\n";
    
            }

        }

    }

    closedir(dir);

    return true;
}
