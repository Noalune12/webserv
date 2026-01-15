#include "ConfigInheritor.hpp"
#include "rules.h"

#include <algorithm>
#include "colors.hpp"
#include <iostream>
#include <sstream>

ConfigInheritor::ConfigInheritor() {}

ConfigInheritor::~ConfigInheritor() {}

void ConfigInheritor::inherit(Tokenizer& tokens) {
    getGlobalDir(tokens.getGlobalDirective());
    getServer(tokens.getVectorContext());
    // printContent();
}


struct MatchFirst {
    const std::string &key;
    MatchFirst(const std::string &k) : key(k) {}

    bool operator()(const std::pair<std::string, std::vector<std::string> >& p) const {
        return p.first == key;
    }
};

void ConfigInheritor::getGlobalDir(PairVector	globalDir) {
    PairVector::iterator it;
    it = std::find_if(globalDir.begin(), globalDir.end(), MatchFirst(ERR_PAGE));
    if (it == globalDir.end()){
        _globalDir.errPage[0] = "default";
    } else {
        setErrorPage(it, _globalDir);
    }

    it = std::find_if(globalDir.begin(), globalDir.end(), MatchFirst(CL_MAX_B_SYZE));
    if (it == globalDir.end()){
        _globalDir.bodySize = 1000;
    } else {
        setBodySize(it, _globalDir);
    }
}

void ConfigInheritor::getServer(std::vector<Context> context) {
    std::vector<Context>::iterator context_it = context.begin();
    for (; context_it != context.end(); context_it++) {
        if (context_it->getIsIgnored())
            continue ;
        PairVector directives = context_it->getDirectives();
        server temp;
        PairVector::iterator it;
        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ERR_PAGE));
        if (it == directives.end()) {
            getErrPageFromGlobal(temp);
        } else {
            setErrorPage(it, temp);
            getErrPageFromGlobal(temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(CL_MAX_B_SYZE));
        if (it == directives.end())
            temp.bodySize = _globalDir.bodySize;
        else {
            setBodySize(it, temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ROOT));
        if (it == directives.end())
            temp.root = "html";
        else {
            temp.root = (*(it->second.begin())).substr(0, (*(it->second.begin())).size() - 1);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(INDEX));
        if (it == directives.end())
            temp.index.push_back("index.html");
        else {
            setIndex(it, temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ALL_METHODS));
        if (it == directives.end()) {
            temp.methods.del = true;
            temp.methods.get = true;
            temp.methods.post = true;
        } else {
            setMethods(it, temp.methods);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(UPLOAD_TO));
        if (it == directives.end())
            temp.uploadTo = "";
        else {
            temp.uploadTo = (*(it->second.begin())).substr(0, (*(it->second.begin())).size() - 1);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(AUTOINDEX));
        if (it == directives.end())
            temp.autoIndex = false;
        else {
            if (*(it->second.begin()) == "off;")
                temp.autoIndex = false;
            else if (*(it->second.begin()) == "on;")
                temp.autoIndex = true;
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(RETURN));
        if (it == directives.end()) {
            temp.ret[0] = "";
        } else {
            setReturn(it, temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(SERV_NAME));
        if (it == directives.end()) {
            temp.serverName.push_back("");
        } else {
            setServerName(it, temp.serverName);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(LISTEN));
        if (it == directives.end()) {
            listenDirective lisTemp;
            lisTemp.ip = "0.0.0.0";
            lisTemp.port = 8080;
            temp.lis.push_back(lisTemp);
        } else {
            setListen(it, temp);
        }

        if (!context.empty())
            getLocation(context_it->getContext(), temp);
        _server.push_back(temp);
    }
}

void ConfigInheritor::getLocation(std::vector<Context>	loc, server& server) {
    std::vector<Context>::iterator loc_it = loc.begin();
    for (; loc_it != loc.end(); loc_it++) {
        location temp;
        std::string name = loc_it->getName().substr(9);
        std::istringstream iss(name);
        iss >> temp.path;
        PairVector directives = loc_it->getDirectives();
        PairVector::iterator it;

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(CGI_PATH));
        if (it == directives.end())
            temp.cgiPath = "";
        else {
            temp.cgiPath = (*(it->second.begin())).substr(0, (*(it->second.begin())).size() - 1);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(CGI_EXT));
        if (it == directives.end())
            temp.cgiExt = "";
        else {
            temp.cgiExt = (*(it->second.begin())).substr(0, (*(it->second.begin())).size() - 1);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ERR_PAGE));
        if (it == directives.end()) {
            getErrPageFromServer(server, temp);
        } else {
            setErrorPage(it, temp);
            getErrPageFromServer(server, temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(CL_MAX_B_SYZE));
        if (it == directives.end())
            temp.bodySize = _globalDir.bodySize;
        else {
            setBodySize(it, temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ROOT));
        PairVector::iterator itAlias = std::find_if(directives.begin(), directives.end(), MatchFirst(ALIAS));
        if (it == directives.end() && itAlias == directives.end()) {
            temp.root = server.root;
            temp.alias = "";
        }
        else if (itAlias != directives.end()) {
            temp.alias = (*(itAlias->second.begin())).substr(0, (*(itAlias->second.begin())).size() - 1);
            temp.root = "";
        }
        else {
            temp.root = (*(it->second.begin())).substr(0, (*(it->second.begin())).size() - 1);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(INDEX));
        if (it == directives.end())
            temp.index = server.index;
        else {
            setIndex(it, temp);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ALL_METHODS));
        if (it == directives.end()) {
            temp.methods.del = server.methods.del;
            temp.methods.get = server.methods.get;
            temp.methods.post = server.methods.post;
        } else {
            setMethods(it, temp.methods);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(UPLOAD_TO));
        if (it == directives.end())
            temp.uploadTo = server.uploadTo;
        else {
            temp.uploadTo = (*(it->second.begin())).substr(0, (*(it->second.begin())).size() - 1);
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(AUTOINDEX));
        if (it == directives.end())
            temp.autoIndex = server.autoIndex;
        else {
            if (*(it->second.begin()) == "off;")
                temp.autoIndex = false;
            else if (*(it->second.begin()) == "on;")
                temp.autoIndex = true;
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(RETURN));
        if (it == directives.end()) {
            getReturnFromServer(server, temp);
        } else {
            setReturn(it, temp);
            getReturnFromServer(server, temp);
        }

        server.loc.push_back(temp);
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

void ConfigInheritor::getErrPageFromServer(server& server, location& location) {
    std::map<int, std::string>::iterator it = server.errPage.begin();

    for (; it != server.errPage.end(); it++) {
        if (location.errPage.find(it->first) ==  location.errPage.end()) {
            location.errPage[it->first] = it->second;
        }
    }
}

void ConfigInheritor::getReturnFromServer(server& server, location& location) {
    std::map<int, std::string>::iterator it = server.ret.begin();

    for (; it != server.ret.end(); it++) {
        if (location.ret.find(it->first) ==  location.ret.end()) {
            location.ret[it->first] = it->second;
        }
    }
}

void ConfigInheritor::setMethods(PairVector::iterator& it, allowMeth& methods) {
    if (std::find(it->second.begin(), it->second.end(), "GET") != it->second.end() \
        || std::find(it->second.begin(), it->second.end(), "GET;") != it->second.end())
        methods.get = true;
    else
        methods.get = false;
    if (std::find(it->second.begin(), it->second.end(), "DELETE") != it->second.end() \
        || std::find(it->second.begin(), it->second.end(), "DELETE;") != it->second.end())
        methods.del = true;
    else
        methods.del = false;
    if (std::find(it->second.begin(), it->second.end(), "POST") != it->second.end() \
        || std::find(it->second.begin(), it->second.end(), "POST;") != it->second.end())
        methods.post = true;
    else
        methods.post = false;
}

template <typename T>
void ConfigInheritor::setErrorPage(PairVector::iterator& it, T& t) {
    std::vector<int> code;
    std::vector<std::string>::iterator itArg = it->second.begin();
    for (; itArg != it->second.end(); itArg++) {
        if (*itArg == " ")
            continue;
        if (itArg->find(';') != std::string::npos) {
            std::string uri = *itArg;
            uri.erase(uri.size() - 1);

            std::vector<int>::iterator itCode = code.begin();
            for (; itCode != code.end(); itCode++) {
                t.errPage[*itCode] = uri;
            }
            code.clear();
        } else {
            int value;
            std::istringstream iss(*itArg);
            iss >> value;
            code.push_back(value);
        }
    }
}

template <typename T>
void ConfigInheritor::setBodySize(PairVector::iterator& it, T& t) {
        std::vector<std::string>::iterator itArg = it->second.begin();
        std::string arg = *itArg;
        std::string s = arg.substr(0, arg.size() - 2);
        std::istringstream iss(s);
        iss >> t.bodySize;
        char suffix = arg[arg.size() - 2];
        switch (std::toupper(suffix)) { // overflow ?
            case 'K': t.bodySize *= 1000; break;
            case 'M': t.bodySize *= 1000000; break;
            case 'G': t.bodySize *= 1000000000; break;
        }
}

template <typename T>
void ConfigInheritor::setIndex(PairVector::iterator& it, T& t) {
    std::vector<std::string>::iterator itArg = it->second.begin();
    for (; itArg != it->second.end(); itArg++) {
        if (itArg == it->second.end() - 1) {
            t.index.push_back((*itArg).substr(0, (*itArg).size() - 1));
        } else {
            t.index.push_back(*itArg);
        }
    }
}

template <typename T>
void ConfigInheritor::setReturn(PairVector::iterator& it, T& t) {
    std::vector<std::string>::iterator itArg = it->second.begin();
    int value;
    for (; itArg != it->second.end(); itArg++) {
        if (*itArg == " ")
            continue;
        if (itArg->find(';') != std::string::npos) {
            t.ret[value] = (*itArg).substr(0, (*itArg).size() - 1);
        } else {
            std::istringstream iss(*itArg);
            iss >> value;
        }
    }
}

void ConfigInheritor::setServerName(PairVector::iterator& it, std::vector<std::string>& serverName) {
    std::vector<std::string>::iterator itArg = it->second.begin();
    for (; itArg != it->second.end(); itArg++) {
        if (*itArg == " ")
            continue;
        if (itArg == it->second.end() - 1) {
            serverName.push_back((*itArg).substr(0, (*itArg).size() - 1));
        } else {
            serverName.push_back(*itArg);
        }
    }
}

void ConfigInheritor::setListen(PairVector::iterator& it, server& s) {
    std::vector<std::string>::iterator itArg = it->second.begin();
    for (; itArg != it->second.end(); itArg++) {
        if (*itArg == " ")
            continue;
        else {
            size_t sepIndex;
            if ((sepIndex = (*itArg).find(":")) != std::string::npos) {
                listenDirective lisTemp;

                lisTemp.ip = (*itArg).substr(0, sepIndex);
                if (lisTemp.ip == "localhost")
                    lisTemp.ip = "127.0.0.1";
                else if (lisTemp.ip == "*")
                    lisTemp.ip = "0.0.0.0";

                std::istringstream iss((*itArg).substr(sepIndex + 1, (*itArg).size()));
                iss >> lisTemp.port;

                s.lis.push_back(lisTemp);

            } else {
                if ((*itArg).find(".") != std::string::npos ||  (*itArg) == "localhost;" || (*itArg) == "*;") {
                    listenDirective lisTemp;
                    lisTemp.ip = (*itArg).substr(0, (*itArg).size() - 1);
                    if (lisTemp.ip == "localhost")
                        lisTemp.ip = "127.0.0.1";
                    else if (lisTemp.ip == "*")
                        lisTemp.ip = "0.0.0.0";
                    lisTemp.port = 8080;
                    s.lis.push_back(lisTemp);
                } else {
                    listenDirective lisTemp;
                    std::istringstream iss(*itArg);
                    iss >> lisTemp.port;
                    lisTemp.ip = "0.0.0.0";
                    s.lis.push_back(lisTemp);
                }
            }
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
        std::vector<std::string>::const_iterator vecstring_it = itt->index.begin();
        for (; vecstring_it != itt->index.end(); vecstring_it++)
            std::cout << *vecstring_it << ", ";
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
        std::cout << "\nserver name: ";
        vecstring_it = itt->serverName.begin();
        for (; vecstring_it != itt->serverName.end(); vecstring_it++)
            std::cout << *vecstring_it << ", ";
        std::cout << std::endl;
        std::vector<listenDirective>::const_iterator lisit = itt->lis.begin();
        std::cout << "listen : ";
        for (; lisit != itt->lis.end(); lisit++) {
            std::cout << "[" << lisit->port << "] - " << lisit->ip << " , ";
        }

        std::cout << GREEN << "\nLOCATION" << RESET << std::endl;
        std::vector<location>::const_iterator itl = itt->loc.begin();
        int j = 1;
        for (; itl != itt->loc.end(); itl++) {
            std::cout << YELLOW << "\nlocation n*" << j << RESET << std::endl;
            std::cout << "path : \'" << itl->path << "\'" << std::endl;
            std::cout << "error page : ";
            std::map<int, std::string>::const_iterator it = itl->errPage.begin();
            for (; it != itl->errPage.end(); it++) {
                std::cout << it->first << " " << it->second << " -- ";
            }
            std::cout << "\nclient max body size : ";
            std::cout << std::fixed << itl->bodySize << " k" << std::endl;
            std::cout << "root : ";
            std::cout << std::fixed << itl->root << std::endl;
            std::cout << "alias : ";
            std::cout << std::fixed << itl->alias << std::endl;
            std::cout << "index : ";
            std::vector<std::string>::const_iterator vecstring_it = itl->index.begin();
            for (; vecstring_it != itl->index.end(); vecstring_it++)
                std::cout << *vecstring_it << ", ";
            std::cout << std::endl;
            std::cout << "allowed methods : ";
            if (itl->methods.del == true) {std::cout << "DEL" << " ";}
            if (itl->methods.get == true) {std::cout << "GET" << " ";}
            if (itl->methods.post == true) {std::cout << "POST with upload to path \'" << itl->uploadTo << "\'";}
            if (itl->methods.del == false && itl->methods.get == false && itl->methods.post == false) {std::cout << "none";}
            std::cout << std::endl;
            std::cout << "auto index : ";
            if (itl->autoIndex == true) {std::cout << "on" << std::endl;} else if (itl->autoIndex == false) {std::cout << "off" << std::endl;}
            std::cout << "return : ";
            it = itl->ret.begin();
            for (; it != itl->ret.end(); it++) {
                std::cout << it->first << " " << it->second << " -- ";
            }
            std::cout << std::endl;
            std::cout << "CGI : " << itl->cgiPath << " with extension " << itl->cgiExt << std::endl;
            j++;
        }
        i++;
    }
}

std::vector<server>&    ConfigInheritor::getServers(void) { return (_server); }
globalDir&              ConfigInheritor::getGlobalDir(void) { return (_globalDir); }

