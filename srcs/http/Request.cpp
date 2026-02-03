#include "Request.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "colors.hpp"

Request::Request(): err(false), status(0), chunkRemaining(false), _keepAlive(false) {}

Request::Request(std::vector<server> servers, globalDir globalDir) : _servers(servers), _globalDir(globalDir), err(false), status(0), chunkRemaining(false), _keepAlive(false) {}


Request::~Request() {}

void Request::printWithoutR(std::string what, std::string line) const {
    std::string l;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] != '\r')
            l.push_back(line[i]);
    }
    std::cout << what <<" = \'" << l << "\' -" << std::endl;
}

void Request::clearPreviousRequest() {
    htmlPage.clear();
    _headersStr.clear();
    _requestLine.clear();
    _body.clear();
    _headers.clear();
    _chunk.clear();
    _reqServer = NULL;
    _reqLocation = NULL;
    _trailing.clear();
    // what about err and keep alive
}

bool Request::hasWS(const std::string& line) const {
    for (size_t i = 0; i < line.length(); i++) {
        if (isspace(line[i]))
            return true;
    }
    return false;
}

bool Request::isOnlyDigits(const std::string& line) const {
	size_t count = 0;

	for (size_t i = 0; i < line.length(); ++i) {
        if (isdigit(line[i]))
            count++;
    }
	if (line.length() == count)
		return true;
	return false;
}

std::string Request::trimOws(const std::string& s)
{
    std::string::size_type start = 0;
    std::string::size_type end = s.size();

    // trim leading spaces/tabs, what about other whitespaces
    while (start < end && (s[start] == ' ' || s[start] == '\t'))
        ++start;

    // trim trailing spaces/tabs, what about other whitespaces
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
        --end;

    return s.substr(start, end - start);
}

void Request::findErrorPage(int code, std::string root, std::map<int, std::string> errPage) {
    err = true;
    status = code;
    std::map<int, std::string>::iterator itErr = errPage.find(code);
    if (itErr == errPage.end())
        return;
    std::string path;
    if (root[root.size() - 1] == '/' && itErr->second[0] == '/')
        path = root.substr(0, root.size() - 1) + itErr->second;
    else if (root[root.size() - 1] != '/' && itErr->second[0] != '/')
        path = root + "/" + itErr->second;
    else
        path = root + itErr->second;

    if (path[0] == '/') // what if many /
        path = path.substr(1, path.size());
    std::cout << "PATH = " << path << std::endl;
			std::ifstream file(path.c_str());
			if (!file) {
				return;
			} else {
				std::cout << "File found" << std::endl;
				std::stringstream buffer;
				buffer << file.rdbuf();
				htmlPage = buffer.str();
			}
}

void Request::methodHandler() {
	// get info
	if (_method == "GET") {
		std::vector<std::string>::iterator itIndex = _reqLocation->index.begin();

        // IF ROOT and no trailing url
        if (!_reqLocation->root.empty()) {
            if (!_trailing.empty()) {
                    std::string path;
                    // if (_uri[_uri.size() - 1] == '/')
                        path = _reqLocation->root + _uri + _trailing;
                    // else
                    //     path = _reqLocation->root + _uri + _trailing;
    
                    if (path[0] == '/')
                        path = path.substr(1, path.size());
                    std::cout << "PATH = " << path << std::endl;

                    struct stat buf;

                    if (stat(path.c_str(), &buf) == 0) {
                        if (!S_ISREG(buf.st_mode)) {
                            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                            std::cout << "error file is not regular" << std::endl;
                            return ;
                        }

                        if (buf.st_mode & S_IRUSR) {
                            std::ifstream file(path.c_str());
                            std::cout << "File found" << std::endl;
                            std::stringstream buffer;
                            buffer << file.rdbuf();
                            htmlPage = buffer.str();
                            err = false;
                            status = 200;
                            return ;
                        } else {
                            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                            std::cout << "error file found but no rights" << std::endl;
                            return ;
                        } 
                    } else { 
                        if (!_reqLocation->root.empty()) {
                            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
                        }
                        else {
                            findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);		
                        }
                        std::cout << "error no file found for trailing file" << std::endl;
                        return ;
                    }
            } else {
                std::string root = _reqLocation->root;
                for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                    std::string path;
                    if (_uri[_uri.size() - 1] == '/')
                        path = root + _uri + *itIndex;
                    else
                        path = root + _uri + "/" + *itIndex;
    
                    if (path[0] == '/')
                        path = path.substr(1, path.size());
                    std::cout << "PATH = " << path << std::endl;

                    struct stat buf;

                    if (stat(path.c_str(), &buf) == 0) { // not sure with many ////
                        if (!S_ISREG(buf.st_mode)) {
                            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                            std::cout << "error file is not regular" << std::endl;
                            return ;
                        }

                        if (buf.st_mode & S_IRUSR) {
                            std::ifstream file(path.c_str());
                            std::cout << "File found" << std::endl;
                            std::stringstream buffer;
                            buffer << file.rdbuf();
                            htmlPage = buffer.str();
                            err = false;
                            status = 200;
                            return ;
                        } else {
                            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
                            std::cout << "error file found but no rights" << std::endl;
                            return ;
                        } 
                    } 
                }
            }
        } else {
            if (!_trailing.empty()) {
                std::cout << "TRYING TO RENDER URI = " << _uri << " with Trailing = " << _trailing << std::endl;
                std::cout << "ALIAS IS " << _reqLocation->alias <<std::endl;
                std::string path = _reqLocation->alias + _trailing;
                if (path[0] == '/')
                    path = path.substr(1, path.size());
                std::cout << "PATH = " << path << std::endl;
                struct stat buf;

                if (stat(path.c_str(), &buf) == 0) {
                    if (!S_ISREG(buf.st_mode)) {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                        std::cout << "error file is not regular" << std::endl;
                        return ;
                    }

                    if (buf.st_mode & S_IRUSR) {
                        std::ifstream file(path.c_str(), std::ios::binary);
                        std::cout << "File found" << std::endl;
                        // std::stringstream buffer;
                        // buffer << file.rdbuf();
                        // htmlPage = buffer.str();
                        htmlPage.assign(
                    	(std::istreambuf_iterator<char>(file)),
                    	std::istreambuf_iterator<char>());
                        err = false;
                        status = 200;
                        return ;
                    } else {
                        findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                        std::cout << "error file found but no rights" << std::endl;
                        return ;
                    } 
                } else { 
                    findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);		
                    std::cout << "error no file found for trailing file" << std::endl;
                    return ;
                }
            } else {
                std::string alias = _reqLocation->alias;
                for (; itIndex != _reqLocation->index.end() ; itIndex++) {
                    std::string path;
                    if (_uri[_uri.size() - 1] == '/')
                        path = alias + *itIndex; 
                    else
                        path = alias + "/" + *itIndex;
                    if (path[0] == '/')
                        path = path.substr(1, path.size());
                    std::cout << "PATH = " << path << std::endl;

                    struct stat buf;

                    if (stat(path.c_str(), &buf) == 0) {
                        if (!S_ISREG(buf.st_mode)) {
                            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                            std::cout << "error file is not regular" << std::endl;
                            return ;
                        }

                        if (buf.st_mode & S_IRUSR) {
                            std::ifstream file(path.c_str());
                            std::cout << "File found" << std::endl;
                            std::stringstream buffer;
                            buffer << file.rdbuf();
                            htmlPage = buffer.str();
                            err = false;
                            status = 200;
                            return ;
                        } else {
                            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
                            std::cout << "error file found but no rights" << std::endl;
                            return ;
                        } 
                    } else { 
                        findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);		
                        std::cout << "error no file found for trailing file" << std::endl;
                        return ;
                    }
                }
            }
        }
	
		if (htmlPage.empty()) {
            if (!_reqLocation->root.empty()) {
                findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
            }
            else {
                findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
            }
        	std::cout << "error no index found" << std::endl;
			return ;
		}
	}
    else if (_method == "DELETE") {
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

void Request::setServerInfo(const int& port, const std::string& ip) {
    _serverIp = ip;
    _serverPort = port;
}
