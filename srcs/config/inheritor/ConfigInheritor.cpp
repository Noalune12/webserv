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
    std::vector<Context>::iterator context_it = context.begin();
    for (; context_it != context.end(); context_it++) {
        std::vector<std::pair<std::string, std::vector<std::string> > > directives = context_it->getDirectives();
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
            temp.root = *(it->second.begin());
            temp.root = temp.root.substr(0, temp.root.size() - 1);
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

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(SERV_NAME));
        if (it == directives.end()) {
            temp.serverName.push_back(""); //default 
        } else {
            std::vector<std::string>::iterator itt = it->second.begin();
            for (; itt != it->second.end(); itt++) {
                if (*itt == " ")
                    continue;
                if (itt == it->second.end() - 1) {
                    std::string arg = *itt;
                    arg = arg.substr(0, arg.size() - 1);
                    temp.serverName.push_back(arg);
                } else {
                    temp.serverName.push_back(*itt);
                }
            }
        }

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(LISTEN));
        if (it == directives.end()) {
            listen lisTemp;
            lisTemp.ip = "*";
            lisTemp.port = 8080;
            temp.lis.push_back(lisTemp);
        } else {
            std::vector<std::string>::iterator itt = it->second.begin();
            for (; itt != it->second.end(); itt++) {
                if (*itt == " ")
                    continue;
                else {
                    std::string arg = *itt;
                    size_t sepIndex;
                    if ((sepIndex = arg.find(":")) != std::string::npos) {
                        //: case -> iP + port
                        listen lisTemp;
                        lisTemp.ip = arg.substr(0, sepIndex);
                        arg = arg.substr(sepIndex + 1, arg.size());
                        int port;
                        std::istringstream iss(arg);
                        iss >> port;
                        lisTemp.port = port;
                        temp.lis.push_back(lisTemp);

                    } else {
                        //if . or localhost (only ip)
                        if (arg.find(".") != std::string::npos ||  arg == "localhost;" || arg == "*;") {
                        // arg.find("localhost") != std::string::npos || arg.find("*") != std::string::npos) {
                            listen lisTemp;
                            lisTemp.ip = arg.substr(0, arg.size() - 1);
                            lisTemp.port = 8080;
                            temp.lis.push_back(lisTemp);
                        } else {
                            listen lisTemp;
                            int port;
                            std::istringstream iss(arg);
                            iss >> port;
                            lisTemp.port = port;
                            lisTemp.ip = "*";
                            temp.lis.push_back(lisTemp);
                        }

                    }
                }    
            }
        }
    
        //location
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
        if (loc_it->getContext().empty())
            std::cout << "\nno more context in" << std::endl;
        std::vector<std::pair<std::string, std::vector<std::string> > > directives = loc_it->getDirectives();
        std::vector<std::pair<std::string, std::vector<std::string> > >::iterator it;
        std::vector<std::pair<std::string, std::vector<std::string> > >::iterator itt;

        // cgi path
        // cgi ext

        // errPage
        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ERR_PAGE));
        if (it == directives.end()) {
            getErrPageFromServer(server, temp);
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
            getErrPageFromServer(server, temp);
        }

        // bodysize
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
                case 'M': temp.bodySize *= 1000; break;
                case 'G': temp.bodySize *= 1000000; break;
            }
        }

        // root & alias
        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ROOT));
        itt = std::find_if(directives.begin(), directives.end(), MatchFirst(ALIAS));
        if (it == directives.end() && itt == directives.end()) {
            temp.root = server.root;
            temp.alias = ""; // not sure
        }
        else if (itt != directives.end()) {
            temp.alias = *(itt->second.begin());
            temp.alias = temp.alias.substr(0, temp.alias.size() - 1);
            temp.root = ""; // not sure
        }
        else {
            temp.root = *(it->second.begin());
            temp.root = temp.root.substr(0, temp.root.size() - 1);
        }

        // index
        it = std::find_if(directives.begin(), directives.end(), MatchFirst(INDEX));
        if (it == directives.end())
            temp.index = server.index;
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
        } // what if there was indexes in server do we add them to the vector of index ???

        // methods

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(ALL_METHODS));
        if (it == directives.end()) {
            temp.methods.del = server.methods.del;
            temp.methods.get = server.methods.get;
            temp.methods.post = server.methods.post;
        } else {
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

        // upload to

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(UPLOAD_TO));
        if (it == directives.end())
            temp.uploadTo = server.uploadTo; // default
        else {
            temp.uploadTo = *(it->second.begin());
            temp.uploadTo = temp.uploadTo.substr(0, temp.uploadTo.size() - 1);
        }

        // auto index

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(AUTOINDEX));
        if (it == directives.end())
            temp.autoIndex = server.autoIndex;
        else {
            std::string arg = *(it->second.begin());
            if (arg == "off;")
                temp.autoIndex = false;
            else if (arg == "on;")
                temp.autoIndex = true;
        }

        // return 

        it = std::find_if(directives.begin(), directives.end(), MatchFirst(RETURN));
        if (it == directives.end()) {
            getReturnFromServer(server, temp);
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
            getReturnFromServer(server, temp); // not sure
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
        std::vector<listen>::const_iterator lisit = itt->lis.begin();
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
            j++;
        }
        i++;
    }
}
