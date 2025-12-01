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

        if (line.empty() || Utils::isOnlyWSpace(line))
            continue;

        std::istringstream iss(line);
        std::string temp;
        line.clear();

        while (iss) {
            iss >> temp;
            line.append(temp);
            line.push_back(' ');
            temp.clear();
        }

        std::size_t	index = line.find('#');
        if (index != std::string::npos) 
                line = line.substr(0, index);
        if (line.empty() || Utils::isOnlyWSpace(line)) 
            continue;

        index = line.find('{'); 

        if (!content.empty() && index != 0) {
            addDirective(content);
            content.clear();
        }

        content.append(line);

        int open;
        if (index != std::string::npos) {
            open = 1;
            std::string name = line;
            std::string contextContent;
            while (getline(f, line)) {
                if (line.find('{') != std::string::npos)
                    open++;
                else if (line.find('}') != std::string::npos)
                    open--;
                contextContent.append(line);
                contextContent.push_back('\n');
                if (open == 0)
                    break;
            }
            Context C(name, contextContent);
            _context.push_back(C);
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

    // Extract directive name (skips whitespace, the >> operator does it by itself)
    if (!(iss >> dir)) {
        return ; // Return here because you already removed the comments in the while (getline) so if its empty then its an empty line
    }

    // Extract all arguments, again skipping whitespace automaticaly
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    // Check for duplicate directives and merge if needed, we can use find with the current token extracted
    // std::map<std::string, std::vector<std::string> >::iterator itm = _directives.find(dir);

	std::vector<std::pair<std::string, std::vector<std::string> > >::iterator itm = _directives.begin();
	for (; itm != _directives.end(); itm++) {
		if (itm->first == dir)
			break ;
	}


    if (itm != _directives.end()) {
        // Directive already exists - append new arguments with separator
        // I use second here to interact directly in the string vector instead of declaring new memory
        itm->second.push_back(" "); // Keeping the separation, might not be needed anympre
        itm->second.insert(itm->second.end(), args.begin(), args.end());
    } else {
        // New directive
		_directives.push_back(std::make_pair(dir, args));
        // _directives[dir] = args;
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