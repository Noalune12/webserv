#include "Context.hpp"

#include <string>
#include <iostream>
#include <sstream>

#include "Utils.hpp"

Context::Context(std::string name, std::string context): _name(name) {
    std::istringstream f(context);
	std::string line;
	std::string content;

    while (getline(f, line)) {

        line = Utils::handleWSpaceComments(line);

        if (line.empty() || Utils::isOnlyWSpace(line))
            continue;

        std::size_t index = line.find('{');

        if (!content.empty() && index != 0) {
            while (!content.empty()) {
                // std::cout << "content : " << content << std::endl;
                std::size_t semiCol = content.find(';');
                if (semiCol == std::string::npos) {
                    addDirective(content);
                    content.clear();
                } else {
                    while (content[semiCol] == ';' || content[semiCol] == ' ')
                        semiCol++;
                    std::string dir = content.substr(0, semiCol);
                    content = content.substr(semiCol);
                    std::cout << "extracted dir : " << dir << "       content : " << content << std::endl;
                    addDirective(dir);
                }
            }
        }

        content.append(line);

        if (index != std::string::npos) {
            _context.push_back(Utils::handleContext(f, content));
            content.clear();
        }
    }

    if (!content.empty()) {
        while (!content.empty()) {
            std::size_t semiCol = content.find(';');
            if (semiCol == std::string::npos) {
                addDirective(content);
                content.clear();
            } else {
                while (content[semiCol] == ';' || content[semiCol] == ' ')
                    semiCol++;
                std::string dir = content.substr(0, semiCol);
                content = content.substr(semiCol);
                addDirective(dir);
            }
        }
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
        if (arg == ";" && !args.empty())
            args.back().append(arg);
        else
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


const std::string&  Context::getName(void) const {
	return (this->_name);
}


const std::vector<std::pair<std::string, std::vector<std::string> > >&  Context::getDirectives(void) const {
	return (this->_directives);
}

const std::vector<Context>& Context::getContext(void) const {
	return (this->_context);
}
