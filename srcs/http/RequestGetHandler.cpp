#include "Request.hpp"
#include "iostream"
#include <sys/stat.h>
#include <sstream>
#include <fstream>

void Request::methodGetHandler() {
    std::vector<std::string>::iterator itIndex = _reqLocation->index.begin();

    if (!_reqLocation->root.empty() && !_trailing.empty()) {

        std::string path = getPath(_reqLocation->root + _uri);
        struct stat buf;

        if (stat(path.c_str(), &buf) == 0) {
            readFile(path, buf, _reqLocation->root);
        } else { 
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
            std::cout << "error no file found for trailing file" << std::endl;
            return ;
        }

    } else if (!_reqLocation->root.empty() && _trailing.empty()) {

        for (; itIndex != _reqLocation->index.end() ; itIndex++) {
            std::string path = getPath(_reqLocation->root + _uri, *itIndex);

            struct stat buf;

            if (stat(path.c_str(), &buf) == 0) {
                if (readFile(path, buf, _reqLocation->root))
                    return ;
            } 
        }
        if (err == true)
            return;
        else {
            findErrorPage(403,  _reqLocation->root, _reqLocation->errPage);
            std::cout << "error GET no file found" << std::endl;
            return ;
        }

    } else if (!_reqLocation->alias.empty() && !_trailing.empty()) {
        std::string path = getPath(_reqLocation->alias);

        struct stat buf;

        if (stat(path.c_str(), &buf) == 0) {
            readFile(path, buf, _reqLocation->root);
        } else { 
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
            std::cout << "error no file found for trailing file with alias" << std::endl;
            return ;
        }

    } else if (!_reqLocation->alias.empty() && _trailing.empty()) {
        for (; itIndex != _reqLocation->index.end() ; itIndex++) {
            std::string path = getPath(_reqLocation->alias, *itIndex);

            struct stat buf;

            if (stat(path.c_str(), &buf) == 0) {
                if (readFile(path, buf, _reqLocation->alias))
                    return ;
            } 
        }
        if (err == true)
            return;
        else {
            findErrorPage(403,  _reqLocation->alias, _reqLocation->errPage);
            std::cout << "error GET no file found" << std::endl;
            return ;
        }
    }

    // if (htmlPage.empty()) {
    //     if (!_reqLocation->root.empty()) {
    //         findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
    //     }
    //     else {
    //         findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);		
    //     }
    // 	std::cout << "error no index found" << std::endl;
    // 	return ;
    // }
}

std::string Request::getPath(std::string folder) {
    std::cout << "ENTERING get Path with only folder " << folder << std::endl;
    std::string path;
    path = folder + _trailing;
    if (path[0] == '/')
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
    if (path[0] == '/')
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
        return false;
    } 
}