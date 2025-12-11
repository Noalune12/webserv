#include "ConfigInheritor.hpp"
#include "rules.h"

#include <algorithm>
#include <iostream>
#include <sstream>

ConfigInheritor::ConfigInheritor() {};

ConfigInheritor::~ConfigInheritor() {};

ConfigInheritor::ConfigInheritor(Tokenizer& tokens) {
    getGlobalDir(tokens.getGlobalDirective());
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
        _globalDir.errPage.code.push_back(0); // default ? 
        _globalDir.errPage.uri = "default"; //default ?
    } else {
        std::vector<std::string>::iterator itt = it->second.begin();
        for (; itt != it->second.end(); itt++) {
            if (itt == it->second.end() - 1) {
                _globalDir.errPage.uri = *itt;
                _globalDir.errPage.uri.erase(_globalDir.errPage.uri.size() - 1);
            } else {
                int value;
                std::istringstream iss(*itt);
                iss >> value;
                _globalDir.errPage.code.push_back(value);
            }
        }
    }

    it = std::find_if(globalDir.begin(), globalDir.end(), MatchFirst(CL_MAX_B_SYZE));
    if (it == globalDir.end()){
        _globalDir.bodySize.size = 0; // default ? 
        _globalDir.bodySize.type = 'X'; //default ?
    } else {
        std::vector<std::string>::iterator itt = it->second.begin();
        std::string arg = *itt;
        std::string s = arg.substr(0, arg.size() - 2);
        std::istringstream iss(s);
        iss >> _globalDir.bodySize.size;
        _globalDir.bodySize.type = arg[arg.size() - 2];
    }
}		

void ConfigInheritor::printContent() const {
    std::cout << "\nINHERITOR FINAL\n" << std::endl;
    std::cout << "GLOBAL DIRECTIVES" << std::endl;

    std::cout << "error page : ";
    std::vector<int>::const_iterator it = _globalDir.errPage.code.begin();
    for (; it != _globalDir.errPage.code.end(); it++) {
        std::cout << *it << " ";
    }
    std::cout << _globalDir.errPage.uri << std::endl;
    std::cout << "client max body size : ";
    std::cout << _globalDir.bodySize.size << " " << _globalDir.bodySize.type << std::endl;
}
