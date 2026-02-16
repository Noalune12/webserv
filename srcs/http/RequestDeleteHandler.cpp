#include "Request.hpp"
#include "Logger.hpp"

#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>

void Request::methodDeleteHandler() {

    Logger::debug("Entering Delete Handler");

    if (_trailing.empty()) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);		
        }
        Logger::warn("Delete: no file or folder");
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

        Logger::debug("Delete: Checking existence and rights for root directory : " + rootDir);
        if (stat(rootDir.c_str(), &buf) != 0) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);		
            }
            Logger::warn("Delete: root directory does not exist");
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
            Logger::warn("Delete: no rights on root directory");
            return ;
        }

        if (stat(path.c_str(), &buf) == 0) {

            if (S_ISREG(buf.st_mode)) {
                Logger::debug("Delete: File " + path + " deleted");
                std::remove(path.c_str());
                err = false;
                status = 204;
            }

            else if (S_ISDIR(buf.st_mode)) {

                Logger::debug("Delete: " + path + " is a Folder");

                DIR* dir = opendir(path.c_str());
                if (dir == NULL) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
                    }
                    Logger::warn("Delete: Open Dir Failed");
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

                            Logger::debug("Delete: " + fullPath + " is a Folder");

                            if (access(fullPath.c_str(), W_OK | X_OK) != 0) {
                                if (!_reqLocation->root.empty()) {
                                    findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                                }
                                else {
                                    findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                                }
                                closedir(dir);

                                Logger::warn("Delete: No rights on directory");
                                return ;
                            }
                            if (!deleteFolder(fullPath)) {
                                closedir(dir);
                                return ;
                            }
                            std::remove(fullPath.c_str());
                        } else if (S_ISREG(st.st_mode)) {
                            Logger::debug("Delete: File " + fullPath + " deleted");
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
                        Logger::warn("Delete: stat failed");
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
            Logger::warn("Delete: file or folder not found");
            return ;
        }
    }
    err = false;
    status = 200;
}

bool Request::deleteFolder(const std::string& path) {

    Logger::debug("Delete: " + path + " is a Folder");

    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(500, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(500, _reqLocation->alias, _reqLocation->errPage);
        }
        Logger::warn("Delete: Open Dir Failed");
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

                Logger::debug("Delete: " + fullPath + " is a Folder");

                if (access(fullPath.c_str(), W_OK | X_OK) != 0) {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                    }
                    Logger::warn("Delete: No rights on directory");
                    closedir(dir);
                    return false;
                }
                if (!deleteFolder(fullPath)) {
                    closedir(dir);
                    return false;
                }
                std::remove(fullPath.c_str());
            } else if (S_ISREG(st.st_mode)) {
                Logger::debug("Delete: File " + fullPath + " deleted");
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
            Logger::warn("Delete: stat failed");
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