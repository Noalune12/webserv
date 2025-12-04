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
const	std::string&  Context::getName(void) const {
	return (this->_name);
}

const	std::vector<std::pair<std::string, std::vector<std::string> > >&  Context::getDirectives(void) const {
	return (this->_directives);
}

const	std::vector<Context>& Context::getContext(void) const {
	return (this->_context);
}

const	Bindings& Context::getBinding(void) const {
	return (this->_bindingsInfo);
}

/* second getter that doesn't return a const so we can modify the data during the validation */
Bindings&	Context::getBinding(void) {
	return (this->_bindingsInfo);
}

/* setter */
void	Context::addListenPair(const std::string& addr, const int& port, const std::string& filePath) {

	if (!_bindingsInfo.checkDuplicateListenPair(addr, port)) {
		_bindingsInfo.listenPairs.push_back(std::make_pair(addr, port));
	} else {
		Utils::logger("temp", filePath);
	}
}

void	Context::addServerName(const std::string& name, const std::string& filePath) {

	if (!_bindingsInfo.checkDuplicateServerName(name)) {
        _bindingsInfo.serverNames.push_back(name);
    } else {
		Utils::logger("temp", filePath);
	}
}

/* utils */
void    Context::printBinding(void) const {

    std::cout << "Listen pairs:" << std::endl;
    for (size_t i = 0; i < _bindingsInfo.listenPairs.size(); ++i) {
        std::cout << "  " << _bindingsInfo.listenPairs[i].first << ":" << _bindingsInfo.listenPairs[i].second << std::endl;
    }

    std::cout << "Server names:" << std::endl;
    if (_bindingsInfo.serverNames.empty()) {
        std::cout << "  (none yet)" << std::endl;
    } else {
        for (size_t i = 0; i < _bindingsInfo.serverNames.size(); ++i) {
            std::cout << "  " << _bindingsInfo.serverNames[i] << std::endl;
        }
    }
}
