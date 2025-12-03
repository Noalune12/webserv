#include "Context.hpp"

#include <string>
#include <iostream>
#include <sstream>

#include "Utils.hpp"

Context::Context(std::string name, std::string context): _name(name), _bindingsInfo() {
    std::istringstream f(context);
	std::string line;
	std::string content;

    while (getline(f, line)) {

        line = Utils::handleWSpaceComments(line);

        if (line.empty() || Utils::isOnlyWSpace(line))
            continue;

        std::size_t index = line.find('{');

        if (!content.empty() && index != 0) {
            addDirective(content);
            content.clear();
        }

        content.append(line);

        if (index != std::string::npos) {
            _context.push_back(Utils::handleContext(f, content));
            content.clear();
        }
    }

    if (!content.empty()) {
        addDirective(content);
        content.clear();
    }
}


Context::~Context() {}

void Context::addDirective(std::string line) {

    std::istringstream iss(line);
    std::string dir;

    if (!(iss >> dir)) {
        return ;
    }

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

	std::vector<std::pair<std::string, std::vector<std::string> > >::iterator itm = _directives.begin();
	for (; itm != _directives.end(); itm++) {
		if (itm->first == dir)
			break ;
	}


    if (itm != _directives.end()) {
        itm->second.push_back(" ");
        itm->second.insert(itm->second.end(), args.begin(), args.end());
    } else {
		_directives.push_back(std::make_pair(dir, args));
    }
}

void Context::printContext() const {
    std::cout << "ENTERING CONTEXT = " << _name << std::endl;
    std::cout << "DIR" << std::endl;
    Utils::printDirectives(_directives);
    std::vector<Context>::const_iterator it;
	for (it = _context.begin(); it != _context.end(); it++) {
		it->printContext();
	}
    std::cout << "LEAVING CONTEXT = " << _name << std::endl;
}

/* getters */
const std::string&  Context::getName(void) const {
	return (this->_name);
}

const std::vector<std::pair<std::string, std::vector<std::string> > >&  Context::getDirectives(void) const {
	return (this->_directives);
}

const std::vector<Context>& Context::getContext(void) const {
	return (this->_context);
}


/* setter */
void    Context::setBindingsInfo(const std::string& addr, const int& port) {

    Bindings tmp;

    tmp.listenPairs.push_back(std::make_pair(addr, port));
    _bindingsInfo.push_back(tmp);


}

/* utils */
void    Context::printBinding(void) {

    static int a = 1;
    std::cout << ++a << std::endl;
    std::vector<Bindings>::iterator it;

    for (it = _bindingsInfo.begin(); it != _bindingsInfo.end(); ++it) {
        std::vector<std::pair<std::string, int> >::iterator itp;
        std::vector<std::string>::iterator  itv;

        for (itp = it->listenPairs.begin(); itp != it->listenPairs.end(); ++itp) {
            std::cout << "address: " << itp->first << std::endl;
            std::cout << "port: " << itp->second << std::endl;
        }
        
        for (itv = it->serverNames.begin(); itv != it->serverNames.end(); ++itv) {
            std::cout << "server_names: " << *itv << std::endl;
        }
    }
}
