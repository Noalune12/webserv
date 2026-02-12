#include "Request.hpp"
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include "colors.hpp"

void Request::methodDeleteHandler() {
    if (_trailing.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);		
        }
        std::cout << "error with DELETE nor file or folder mentionned" << std::endl;
        return ;
    } else {
        std::string  path;
        if (!_reqLocation->root.empty())
            path = getPath(_reqLocation->root + _uri, _trailing);
        else
            path = getPath(_reqLocation->alias, _trailing);
        std::string  rootDir = getDirectory();
        if (path[0] == '/')
            path = path.substr(1, path.size());
        if (rootDir[0] == '/')
            rootDir = rootDir.substr(1, rootDir.size());

        struct stat buf;
        if (stat(rootDir.c_str(), &buf) != 0) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);		
            }
            std::cout << "error with DELETE file or folder note found" << std::endl;
            return ;
        }


        // delete permission based on directory not file
        if (access(rootDir.c_str(), W_OK | X_OK) != 0) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
            }
            std::cout << "error with DELETE no rights on root directoru" << std::endl;
            return ;
        }

        if (stat(path.c_str(), &buf) == 0) {

            if (S_ISREG(buf.st_mode)) {
                std::remove(path.c_str());
                err = false;
                status = 204;
            }

            else if (S_ISDIR(buf.st_mode)) {

                DIR* dir = opendir(path.c_str());
                if (dir == NULL) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
                    }
                    std::cout << "error with DELETE opendir failed" << std::endl;
                    return ;
                }
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string name = entry->d_name;
                    if (name == "." || name == "..")
                        continue;
                    std::string fullPath;
                    if (path[path.size() - 1] != '/')
                        fullPath = path + "/" + name;
                    else
                        fullPath = path + name;

                    struct stat st;
                    if (stat(fullPath.c_str(), &st) == 0) {

                        if (S_ISDIR(st.st_mode)) {
                                if (access(fullPath.c_str(), W_OK | X_OK) != 0) {
                                    if (!_reqLocation->root.empty()) {
                                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                                    }
                                    else {
                                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                                    }
                                    closedir(dir);
                                    std::cout << "error with DELETE no rights on a directory inside what is to be deleted" << std::endl;
                                    return ;
                                }
                                if (!deleteFolder(fullPath)) {
                                    closedir(dir);
                                    return ;
                                }
                                std::remove(fullPath.c_str());
                        } else if (S_ISREG(st.st_mode)) {
                            std::remove(fullPath.c_str());
                        } else {
                            std::remove(fullPath.c_str());
                        }

                    } else {
                        closedir(dir);
                        if (!_reqLocation->root.empty()) {
                            findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
                        }
                        else {
                            findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
                        }
                        std::cout << "error with DELETE stats failed" << std::endl;
                        return ;
                    }

                }
                closedir(dir);
                std::remove(path.c_str());
                err = false;
                status = 204;
            }
        } else {
            if (!_reqLocation->root.empty()) {
                findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);		
            }
            std::cout << "error with DELETE file or folder note found" << std::endl;
            return ;
        }
    }
    err = false;
    status = 200;
}

bool Request::deleteFolder(const std::string& path) {

    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with DELETE opendir failed" << std::endl;
        return false;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {

        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        std::string fullPath;
        if (path[path.size() - 1] != '/')
            fullPath = path + "/" + name;
        else
            fullPath = path + name;

        struct stat st;

        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (access(fullPath.c_str(), W_OK | X_OK) != 0) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                    }
                    std::cout << "error with DELETE no rights on a directory inside what is to be deleted" << std::endl;
                    closedir(dir);
                    return false;
                }
                if (!deleteFolder(fullPath)) {
                    closedir(dir);
                    return false;
                }
                std::remove(fullPath.c_str());
            } else if (S_ISREG(st.st_mode)) {
                std::remove(fullPath.c_str());
    
            } else {
                std::remove(fullPath.c_str());
            }
        } else {
            closedir(dir);
            if (!_reqLocation->root.empty()) {
                findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
            }
            std::cout << "error with DELETE stats failed" << std::endl;
            return false;
        }

    }
    closedir(dir);
    err = false;
    status = 204;
    return true;
}


std::string Request::getDirectory() {

    std::string ret;

    if (!_reqLocation->root.empty()) 
        ret = _reqLocation->root + _uri;
    else
        ret = _reqLocation->alias;

    std::string trail = _trailing;
    size_t index = trail.find('/');

    while (index != std::string::npos) {

        if (ret[ret.size() - 1] != '/')
            ret = ret + "/" + trail.substr(0, index + 1);
        else
            ret += trail.substr(0, index + 1);

        trail = trail.substr(index + 1);
        index = trail.find('/');
    }
    return ret;
}