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
    
    if (_reqLocation->uploadTo.empty()) {
        if (!_reqLocation->root.empty())
            _postUploadDir = _reqLocation->root;
        else
            _postUploadDir = _reqLocation->alias;
    } else {
        _postUploadDir = _reqLocation->uploadTo;
    }
    if (_postUploadDir[0] == '/') 
        _postUploadDir = _postUploadDir.substr(1, _postUploadDir.size());
    std::cout << "UPLOAD TO " << _postUploadDir << std::endl;
    // check rights

    struct stat buf;
    if (stat(_postUploadDir.c_str(), &buf) != 0) {
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
    if (access(_postUploadDir.c_str(), W_OK | X_OK) != 0) {
        if (!_reqLocation->root.empty())
            findErrorPage(403, _reqLocation->root, _reqLocation->errPage);
        else
            findErrorPage(403, _reqLocation->alias, _reqLocation->errPage);
        std::cout << "error with POST, no right on folder" << std::endl;
        return ;  
    }

    if (_postUploadDir[_postUploadDir.size() - 1] != '/') {
        _postUploadDir = _postUploadDir + "/";
    }

    // create file
    double add = 1;
    DIR* dir = opendir(_postUploadDir.c_str()); // protect
    struct dirent* entry;
    std::vector<std::string> filesExt;
    while ((entry = readdir(dir)) != NULL) {
        std::string n = entry->d_name;
        if (n == "." || n == "..")
            continue;

        struct stat st;
        // std::cout << "checking : " << _postUploadDir + n << std::endl;
        stat((_postUploadDir + n).c_str(), &st); //protection
        if (!S_ISREG(st.st_mode))
            continue;
        // std::cout << "file name in upload dir = " << n << std::endl;
        size_t index = n.find("upload_");
        // std::cout << "upload_ index = " << index << std::endl;
        if (index == std::string::npos || index != 0)
            continue;

        std::string temp = n.substr(7);
        
        index = temp.find(".");
        if (index != std::string::npos) {
            temp = temp.substr(0, index);
        }
        // std::cout << "after upload_ = " << temp << std::endl;
        filesExt.push_back(temp);

    }
    closedir(dir);

    std::sort(filesExt.begin(), filesExt.end());

    std::vector<std::string>::iterator index;
    std::stringstream ss;
    ss << add;
    std::string addStr = ss.str();
    // std::cout << "Comparing temp and addStr : " << addStr << std::endl;
    while ((index = std::find(filesExt.begin(), filesExt.end(), addStr)) != filesExt.end()) {
        std::stringstream sss;
        add++;
        // if (add too big)
        sss << add;
        addStr = sss.str();
        // std::cout << "Comparing temp and addStr : " << addStr << std::endl;
    }

    // ss << add;
    // std::string addStr = ss.str();

    _postFilename = "upload_" + addStr + _postExt;


    std::cout << "FILENAME = " << _postFilename << "  in directory = " << _postUploadDir << std::endl;
    
    // put body into file

    std::ofstream outfile ((_postUploadDir + _postFilename).c_str());

    outfile << "my text here!" << std::endl;

    outfile.close();
    
    
    
    


    std::cout << std::endl;
}