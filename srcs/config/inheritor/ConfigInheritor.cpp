#include "ConfigInheritor.hpp"
#include "rules.h"
#include <colors.h>

#include <algorithm>
#include <iostream>
#include <sstream>

ConfigInheritor::ConfigInheritor() {};

ConfigInheritor::~ConfigInheritor() {};

ConfigInheritor::ConfigInheritor(Tokenizer& tokens) {
    getGlobalDir(tokens.getGlobalDirective());
    getServer(tokens.getVectorContext());
    printContent();
}


struct MatchFirst {
    const std::string &key;
    MatchFirst(const std::string &k) : key(k) {}

    bool operator()(const std::pair<std::string, std::vector<std::string> >& p) const {
        return p.first == key;
    }
};

void ConfigInheritor::getGlobalDir(std::vector<std::pair<std::string, std::vector<std::string> > >	globalDir) {
    std::vector<std::pair<std::string, std::vector<std::string> > >::iterator it;
    it = std::find_if(globalDir.begin(), globalDir.end(), MatchFirst(ERR_PAGE));
    if (it == globalDir.end()){
        _globalDir.errPage[0] = "default"; //default ?
    } else {
        std::vector<std::string>::iterator itt = it->second.begin();
        std::vector<int> code;
        for (; itt != it->second.end(); itt++) {
            if (*itt == " ")
                continue;
            if (itt->find(';') != std::string::npos) {
                std::string uri = *itt;
                uri.erase(uri.size() - 1);

                std::vector<int>::iterator ittt = code.begin();
                for (; ittt != code.end(); ittt++) {
                    _globalDir.errPage[*ittt] = uri;
                }
                code.clear();
            } else {
                int value;
                std::istringstream iss(*itt);
                iss >> value;
                code.push_back(value);
            }
        }
    }

    it = std::find_if(globalDir.begin(), globalDir.end(), MatchFirst(CL_MAX_B_SYZE));
    if (it == globalDir.end()){
        _globalDir.bodySize = 1000; // default ? 
    } else {
        std::vector<std::string>::iterator itt = it->second.begin();
        std::string arg = *itt;
        std::string s = arg.substr(0, arg.size() - 2);
        std::istringstream iss(s);
        iss >> _globalDir.bodySize;
        char suffix = arg[arg.size() - 2];
        switch (std::toupper(suffix)) { // overflow ?
            // case 'K': _globalDir.bodySize *= 1024; break;
            // case 'M': _globalDir.bodySize *= 1024 * 1024; break;
            // case 'G': _globalDir.bodySize *= 1024 * 1024 * 1024; break;
            case 'M': _globalDir.bodySize *= 1000; break;
            case 'G': _globalDir.bodySize *= 1000000; break;
        }
    }
}

void ConfigInheritor::getServer(std::vector<Context> context) {
    std::vector<Context>::iterator it = context.begin();
    for (; it != context.end(); it++) {
        std::vector<std::pair<std::string, std::vector<std::string> > > directives = it->getDirectives();
        server temp;
        std::vector<std::pair<std::string, std::vector<std::string> > >::iterator it;
        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ERR_PAGE));
        if (it == directives.end()) {
            getErrPageFromGlobal(temp);
        } else {
            std::vector<std::string>::iterator itt = it->second.begin();
            std::vector<int> code;
            for (; itt != it->second.end(); itt++) {
                if (*itt == " ")
                    continue;
                if (itt->find(';') != std::string::npos) {
                    std::string uri = *itt;
                    uri.erase(uri.size() - 1);

                    std::vector<int>::iterator ittt = code.begin();
                    for (; ittt != code.end(); ittt++) {
                        temp.errPage[*ittt] = uri;
                    }
                    code.clear();
                } else {
                    int value;
                    std::istringstream iss(*itt);
                    iss >> value;
                    code.push_back(value);
                }
            }
            getErrPageFromGlobal(temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(CL_MAX_B_SYZE));
        if (it == directives.end())
            temp.bodySize = _globalDir.bodySize;
        else {
            std::vector<std::string>::iterator itt = it->second.begin();
            std::string arg = *itt;
            std::string s = arg.substr(0, arg.size() - 2);
            std::istringstream iss(s);
            iss >> temp.bodySize;
            char suffix = arg[arg.size() - 2];
            switch (std::toupper(suffix)) { // overflow ?
                // case 'K': temp.bodySize *= 1024; break;
                // case 'M': temp.bodySize *= 1024 * 1024; break;
                // case 'G': temp.bodySize *= 1024 * 1024 * 1024; break;
                case 'M': temp.bodySize *= 1000; break;
                case 'G': temp.bodySize *= 1000000; break;
            }
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ROOT));
        if (it == directives.end())
            temp.root = "html"; // default 
        else {
            std::vector<std::string>::iterator itt = it->second.begin();
            temp.root = *itt;
            temp.root = temp.root.substr(0, temp.root.size() - 1);
            // temp.root = arg;
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(INDEX));
        if (it == directives.end())
            temp.index.push_back("index.html"); // default 
        else {
            std::vector<std::string>::iterator itt = it->second.begin();
            for (; itt != it->second.end(); itt++) {
                if (itt == it->second.end() - 1) {
                    std::string arg = *itt;
                    arg = arg.substr(0, arg.size() - 1);
                    temp.index.push_back(arg);
                } else {
                    temp.index.push_back(*itt);
                }
            }
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ALL_METHODS));
        // temp.methods.del = false;
        // temp.methods.get = false;
        // temp.methods.post = false;
        if (it == directives.end()) {
            temp.methods.del = true;
            temp.methods.get = true;
            temp.methods.post = true;
        } else {
            // std::vector<std::string>::iterator itt = it->second.begin();
            // for (; itt != it->second.end(); itt++) {
            //     if (*itt == "GET" || *itt == "GET;")
            //         temp.methods.get = true;
            //     else if (*itt == "POST" || *itt == "POST;")
            //         temp.methods.post = true;
            //     else if (*itt == "DELETE" || *itt == "DELETE;")
            //         temp.methods.del = true;
            // }
            if (std::find(it->second.begin(), it->second.end(), "GET") != it->second.end() \
                || std::find(it->second.begin(), it->second.end(), "GET;") != it->second.end())
                temp.methods.get = true;
            else
                temp.methods.get = false;
            if (std::find(it->second.begin(), it->second.end(), "DELETE") != it->second.end() \
                || std::find(it->second.begin(), it->second.end(), "DELETE;") != it->second.end())
                temp.methods.del = true;
            else
                temp.methods.del = false;
            if (std::find(it->second.begin(), it->second.end(), "POST") != it->second.end() \
                || std::find(it->second.begin(), it->second.end(), "POST;") != it->second.end())
                temp.methods.post = true;
            else
                temp.methods.post = false;
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(UPLOAD_TO));
        if (it == directives.end())
            temp.uploadTo = ""; // default
        else {
            temp.uploadTo = *(it->second.begin());
            temp.uploadTo = temp.uploadTo.substr(0, temp.uploadTo.size() - 1);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(AUTOINDEX));
        if (it == directives.end())
            temp.autoIndex = false;
        else {
            std::string arg = *(it->second.begin());
            if (arg == "off;")
                temp.autoIndex = false;
            else if (arg == "on;")
                temp.autoIndex = true;
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(RETURN));
        if (it == directives.end()) {
            temp.ret[0] = "";
        } else {
            std::vector<std::string>::iterator itt = it->second.begin();
            int value;
            for (; itt != it->second.end(); itt++) {
                if (*itt == " ")
                    continue;
                if (itt->find(';') != std::string::npos) {
                    std::string url = *itt;
                    url.erase(url.size() - 1);
                    temp.ret[value] = url;
                } else {
                    std::istringstream iss(*itt);
                    iss >> value;
                }
            }
        }
        //listen
        //servername
    
        //location

        _server.push_back(temp);
    }
}

void ConfigInheritor::getErrPageFromGlobal(server& server) {
    std::map<int, std::string>::iterator it = _globalDir.errPage.begin();

    for (; it != _globalDir.errPage.end(); it++) {
        if (server.errPage.find(it->first) ==  server.errPage.end()) {
            server.errPage[it->first] = it->second;
        }
    }
}

void ConfigInheritor::printContent() const {
    std::cout << RED << "\nINHERITOR FINAL\n" << RESET << std::endl;
    std::cout << BLUE << "GLOBAL DIRECTIVES" << RESET << std::endl;

    std::cout << "error page : ";
    std::map<int, std::string>::const_iterator it = _globalDir.errPage.begin();
    for (; it != _globalDir.errPage.end(); it++) {
        std::cout << it->first << " " << it->second << " -- ";
    }
    std::cout << "\nclient max body size : ";
    std::cout << std::fixed <<_globalDir.bodySize << " k" << std::endl;

    std::cout << GREEN << "\nSERVER" << RESET << std::endl;
    std::vector<server>::const_iterator itt = _server.begin();
    int i = 1;
    for (; itt != _server.end(); itt++) {
        std::cout << YELLOW << "\nsevrer n*" << i << RESET << std::endl;
        std::cout << "error page : ";
        std::map<int, std::string>::const_iterator it = itt->errPage.begin();
        for (; it != itt->errPage.end(); it++) {
            std::cout << it->first << " " << it->second << " -- ";
        }
        std::cout << "\nclient max body size : ";
        std::cout << std::fixed << itt->bodySize << " k" << std::endl;
        std::cout << "\nroot : ";
        std::cout << std::fixed << itt->root << std::endl;
        std::cout << "index : ";
        std::vector<std::string>::const_iterator index_it = itt->index.begin();
        for (; index_it != itt->index.end(); index_it++)
            std::cout << *index_it << ", ";
        std::cout << std::endl;
        std::cout << "allowed methods : ";
        if (itt->methods.del == true) {std::cout << "DEL" << " ";} 
        if (itt->methods.get == true) {std::cout << "GET" << " ";} 
        if (itt->methods.post == true) {std::cout << "POST with upload to path \'" << itt->uploadTo << "\'";} 
        if (itt->methods.del == false && itt->methods.get == false && itt->methods.post == false) {std::cout << "none";}
        std::cout << std::endl;
        std::cout << "auto index : ";
        if (itt->autoIndex == true) {std::cout << "on" << std::endl;} else if (itt->autoIndex == false) {std::cout << "off" << std::endl;}
        std::cout << "return : ";
        it = itt->ret.begin();
        for (; it != itt->ret.end(); it++) {
            std::cout << it->first << " " << it->second << " -- ";
        }
        i++;
    }
}
