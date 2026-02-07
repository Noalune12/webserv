#include "Request.hpp"
#include "iostream"
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include "colors.hpp"
#include "unistd.h"
void Request::methodGetHandler() {
    if (_reqLocation->autoIndex == true)
        std::cout << "\nAUTOINDEX IS ON" << std::endl;
    else
        std::cout << "\nAUTOINDEX IS OFF" << std::endl;

    if (!_reqLocation->root.empty() && !_trailing.empty()) {
        _getPath = getPath(_reqLocation->root + _uri, _trailing);
    } else if (!_reqLocation->root.empty() && _trailing.empty()) {
        _getPath = getPath(_reqLocation->root + _uri);
    } else if (!_reqLocation->alias.empty() && !_trailing.empty()) {
        _getPath = getPath(_reqLocation->alias, _trailing);
    } else if (!_reqLocation->alias.empty() && _trailing.empty()) {
        _getPath = getPath(_reqLocation->alias);
    }

    std::cout << "PATH to access index is " << _getPath << std::endl;

    std::vector<std::string>::iterator itIndex = _reqLocation->index.begin();

    struct stat buf;
    if (stat(_getPath.c_str(), &buf) == 0) {
        if (_trailing.empty()) {
            if (S_ISDIR(buf.st_mode)) {
                std::cout << "no trailing: path is a folder" << std::endl;
                if (access(_getPath.c_str(), W_OK | X_OK) != 0) {
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
                        if (readFile(path, st, _reqLocation->root))
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
            if (S_ISDIR(buf.st_mode)) {
                std::cout << "trailing: path is a folder" << std::endl;
                if (access(_getPath.c_str(), W_OK | X_OK) != 0) {
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
                        if (readFile(path, st, _reqLocation->root))
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

            } else if (S_ISREG(buf.st_mode)) {
                if (readFile(_getPath, buf, _reqLocation->root)) {
                    return ;
                } else {
                    if (!_reqLocation->root.empty()) {
                        findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                    }
                    else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
                    }
                    std::cout << "error no file found for trailing file" << std::endl;
                    return ;
                }
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
    std::cout << "ENTERING get Path with only folder " << folder << std::endl;
    std::string path;
    if (folder[folder.size() - 1] != '/' && !_trailing.empty())
        path = folder + "/" + _trailing;
    else
        path = folder + _trailing;
    if (path[0] == '/') // not sure
        path = path.substr(1, path.size());
    std::cout << "PATH = " << path << std::endl;
    return path;
}

std::string Request::getPath(std::string folder, std::string file) {
    std::cout << "ENTERING get Path with  folder " << folder << " and trailing " << file << std::endl;
    std::string path;
    if (folder[folder.size() - 1] == '/')
        path = folder + file;
    else
        path = folder + "/" + file;
    if (path[0] == '/') // not sure
        path = path.substr(1, path.size());
    std::cout << "PATH = " << path << std::endl;
    return path;
}

bool Request::readFile(std::string path, struct stat buf ,std::string errorPath) {


    if (!S_ISREG(buf.st_mode)) {
        findErrorPage(403, errorPath, _reqLocation->errPage);
        std::cout << "error file is not regular" << std::endl;
        return false;
    }

    if (buf.st_mode & S_IRUSR) {
        std::ifstream file(path.c_str());
        std::cout << "File found" << std::endl;
        std::stringstream buffer;
        buffer << file.rdbuf();
        htmlPage = buffer.str();
        err = false;
        status = 200;
        return true;
    } else {
        findErrorPage(403, errorPath, _reqLocation->errPage);
        std::cout << "error file found but no rights" << std::endl;
        _indexFound = true;
        return false;
    }
}

bool Request::handleAutoindex(std::string dirPath) {

    if (access(dirPath.c_str(), W_OK | X_OK) != 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        }
        std::cout << "error with GET AUTOINDEX no rights on a directory" << std::endl;
        return false;
    }
    htmlPage = "<!DOCTYPE html>\n<html>\n<head>\n <meta charset=\"UTF-8\">\n<title>Index</title>\n</head><h1>Index of " + dirPath + "\n\n</h1>";


    DIR* dir = opendir(dirPath.c_str()); // protect

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
        stat(fullPath.c_str(), &st); // protection ??
        if (S_ISDIR(st.st_mode)) {
            std::cout << YELLOW "folder : " << fullPath << RESET << std::endl;
            htmlPage += "<a href=\"" + name + "/\">📁 " + name + "</a><br>\n";
        } else if (S_ISREG(st.st_mode)) {
            htmlPage += "<a href=\"" + name + "\">📄 " + name + "</a><br>\n";
            std::cout << BLUE "regular file :" << fullPath << RESET << std::endl;

        } else {
            // what do I do ?
            std::cout << GREEN "not regular file " << fullPath << RESET << std::endl;
        }
    }
    closedir(dir);

    return true;
}
