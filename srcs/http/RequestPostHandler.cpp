#include "Request.hpp"
#include <iostream>
#include "MimeTypes.hpp"
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>

void Request::methodPostHandler() {
    std::cout << "\nENTERING POST HANDLER" << std::endl; 
    if (_body.empty()) {
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
    std::map<std::string, std::string>::iterator it = _headers.find("content-type");
    if (it == _headers.end()) {
        _postExt = ".txt"; // not sure
    } else {
        // set extenstion

        // check is valid extension in mime types ?
        size_t		lastSlash = it->second.find_last_of('/');
        _postExt = ".";
        if (lastSlash != std::string::npos) {
            _postExt += it->second.substr(lastSlash + 1);
        } else {
            // error
        }
        
    }
    std::cout << "EXTENSION = " << _postExt << std::endl;

    // find location
    
    std::string uploadDir;
    if (_reqLocation->uploadTo.empty()) {
        if (!_reqLocation->root.empty())
            uploadDir = _reqLocation->root;
        else
            uploadDir = _reqLocation->alias;
    } else {
        uploadDir = _reqLocation->uploadTo;
    }
    if (uploadDir[0] == '/') 
        uploadDir = uploadDir.substr(1, uploadDir.size());
    std::cout << "UPLOAD TO " << uploadDir << std::endl;
    // check rights

    struct stat buf;
    if (stat(uploadDir.c_str(), &buf) != 0) {
        if (!_reqLocation->root.empty()) {
            findErrorPage(404, _reqLocation->root, _reqLocation->errPage);
        }
        else {
            findErrorPage(404, _reqLocation->alias, _reqLocation->errPage);		
        }
        std::cout << "error with POST folder does not exist" << std::endl;
        return ;
    }
    if (!S_ISDIR(buf.st_mode)) {
        if (!_reqLocation->root.empty())
            findErrorPage(400, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(400, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, upload to is not a folder" << std::endl;
        return ;  
    }
    if (access(uploadDir.c_str(), W_OK | X_OK) != 0) {
        if (!_reqLocation->root.empty())
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, no right on folder" << std::endl;
        return ;  
    }

    // create file
    // std::string name;
    // double add;
    // DIR* dir = opendir(uploadDir.c_str()); // protect
    // struct dirent* entry;
    // while ((entry = readdir(dir)) != NULL) {
    //     std::string n = entry->d_name;
    // }
    // if (uploadDir[uploadDir.size() - 1] != '/') {
    //     _postFilename = uploadDir + "/" + name;
    // } else {
    //     _postFilename = uploadDir + name;
    // }

    // put body into file

    std::cout << std::endl;
}