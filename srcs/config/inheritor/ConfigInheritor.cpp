#include "ConfigInheritor.hpp"
#include "rules.h"

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
        _globalDir.bodySize = 0; // default ? 
    } else {
        std::vector<std::string>::iterator itt = it->second.begin();
        std::string arg = *itt;
        std::string s = arg.substr(0, arg.size() - 2);
        std::istringstream iss(s);
        iss >> _globalDir.bodySize;
        char suffix = arg[arg.size() - 2];
        switch (std::toupper(suffix)) { // overflow ?
            case 'K': _globalDir.bodySize *= 1024; break;
            case 'M': _globalDir.bodySize *= 1024 * 1024; break;
            case 'G': _globalDir.bodySize *= 1024 * 1024 * 1024; break;
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

        // bodysize
    
        //listen
        //servername
        //root
        // index
        //autoindex
        //uploadto
        //return
    
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
    std::cout << "\nINHERITOR FINAL\n" << std::endl;
    std::cout << "GLOBAL DIRECTIVES" << std::endl;

    std::cout << "error page : ";
    std::map<int, std::string>::const_iterator it = _globalDir.errPage.begin();
    for (; it != _globalDir.errPage.end(); it++) {
        std::cout << it->first << " " << it->second << " -- ";
    }
    std::cout << "\nclient max body size : ";
    std::cout << std::fixed <<_globalDir.bodySize << " k" << std::endl;

    std::cout << "\nSERVER" << std::endl;
    std::vector<server>::const_iterator itt = _server.begin();
    for (; itt != _server.end(); itt++) {
        std::cout << "error page : ";
        std::map<int, std::string>::const_iterator it = itt->errPage.begin();
        for (; it != itt->errPage.end(); it++) {
            std::cout << it->first << " " << it->second << " -- ";
        }
    }
}
