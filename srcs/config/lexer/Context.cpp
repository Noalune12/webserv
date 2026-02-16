#include "Context.hpp"

#include <string>
#include <iostream>
#include <sstream>

#include "colors.hpp"
#include "Utils.hpp"

Context::Context(std::string name, std::string context): _name(name), _bindingsInfo(), _isIgnored(false) {
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

		content.append(line);

        if (index != std::string::npos) {
            _context.push_back(Utils::handleContext(f, content));
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

    size_t index = dir.find(';');

    std::vector<std::string> args;
    std::string arg;

    if (index != 0 && index != std::string::npos) {
        args.push_back(dir.substr(index, dir.size()));
        dir = dir.substr(0, index);
    }

    while (iss >> arg) {
        if (arg[0] == ';' && !args.empty())
            args.back().append(arg);
        else
            args.push_back(arg);
    }

	PairVector::iterator itm = _directives.begin();
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
const	std::string&  Context::getName(void) const {
	return (this->_name);
}

const	PairVector&  Context::getDirectives(void) const {
	return (this->_directives);
}

const	std::vector<Context>& Context::getContext(void) const {
	return (this->_context);
}

std::vector<Context>& Context::getContext(void) {
	return (this->_context);
}

const	Bindings& Context::getBinding(void) const {
	return (this->_bindingsInfo);
}

bool    Context::getIsIgnored(void) const {
    return (this->_isIgnored);
}

void    Context::setIsIgnored(void) {
    this->_isIgnored = true;
}

/* second getter that doesn't return a const so we can modify the data during the validation */
Bindings&	Context::getBinding(void) {
	return (this->_bindingsInfo);
}

/* setter */
void	Context::addListenPair(const std::string& addr, const int& port, const std::string& filePath) {

	if (_bindingsInfo.checkDuplicateListenPair(addr, port)) {
		std::ostringstream oss;
		oss << port;
		std::string errorMsg = "a duplicate listen \"" + addr + ":" + oss.str() + "\"";
		Utils::logger(errorMsg, filePath);
		throw std::invalid_argument(errorMsg);
	}
	_bindingsInfo.listenPairs.push_back(std::make_pair(addr, port));
}

void	Context::addServerName(const std::string& name, const std::string& filePath) {
	(void) filePath;
	if (_bindingsInfo.checkDuplicateServerName(name)) {
		return ;
	}
	_bindingsInfo.serverNames.push_back(name);
}
