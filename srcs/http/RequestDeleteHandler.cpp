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
    } else if (!_reqLocation->root.empty()) {
        std::string  path = _reqLocation->root + _uri + _trailing;
        std::string  rootDir = getDirectory();
        if (path[0] == '/')
            path = path.substr(1, path.size());
        if (rootDir[0] == '/')
            rootDir = rootDir.substr(1, rootDir.size());
        std::cout << "PATH = " << path << " DIR = " << rootDir << std::endl;
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
        // check permission of last folder (can be in trailing)
        

        if (stat(path.c_str(), &buf) == 0) {
            if (S_ISREG(buf.st_mode)) {
                // regular file
                htmlPage = "File found for delete";
                std::remove(path.c_str());
                err = false;
                status = 204;
            }
            else if (S_ISDIR(buf.st_mode)) {
                // directory
                // opendir, readdir and closedir
                DIR* dir = opendir(path.c_str()); // protect
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string name = entry->d_name;
                    if (name == "." || name == "..")
                        continue;
                    std::string fullPath = path + name;

                    struct stat st;
                    stat(fullPath.c_str(), &st); // protection ??
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
                            std::cout << YELLOW "folder : " << fullPath << RESET << std::endl;
                            // get in folder
                            if (!deleteFolder(fullPath)) {
                            closedir(dir);
                                return ;
                            }
                            std::remove(fullPath.c_str());
                    } else if (S_ISREG(st.st_mode)) {
                        std::remove(fullPath.c_str());
                        std::cout << BLUE "regular file :" << fullPath << RESET << std::endl;

                    } else {
                        // what do I do ?
                        std::cout << GREEN "not regular file " << fullPath << RESET << std::endl;
                    }

                }
                closedir(dir);
                 std::remove(path.c_str());
                htmlPage = "I have to delete a directory";
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
    } else if (!_reqLocation->alias.empty()) {

    }
}

bool Request::deleteFolder(std::string path) {
        DIR* dir = opendir(path.c_str()); // protect
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
                continue;
            std::string fullPath = path + "/" + name;

            struct stat st;
            stat(fullPath.c_str(), &st); // protection ??
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
                std::cout << YELLOW "folder : " << fullPath << RESET << std::endl;
                // get in folder
                if (!deleteFolder(fullPath)) {
                    closedir(dir);
                    return false;
                }
                std::remove(fullPath.c_str());
            } else if (S_ISREG(st.st_mode)) {
                std::cout << BLUE "regular file :" << fullPath << RESET << std::endl;
                std::remove(fullPath.c_str());

            } else {
                // what do I do ?
                std::cout << GREEN "not regular file " << fullPath << RESET << std::endl;
            }

        }
        closedir(dir);
        err = false;
        status = 204;
        return true;
}


std::string Request::getDirectory() {
    std::string ret = _reqLocation->root + _uri;
    std::string trail = _trailing;
    size_t index = trail.find('/');
    while (index != std::string::npos) {
        std::cout << "INDEX = " << index << std::endl;
        ret += trail.substr(0, index + 1);
        trail = trail.substr(index + 1);
        std::cout << RED "NEW DIR = " << ret << " and TRAIL = " << trail << RESET << std::endl;
        index = trail.find('/');
    }
    return ret;
}