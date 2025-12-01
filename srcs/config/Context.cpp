#include "Context.hpp"

#include <string>
#include <iostream>
#include <sstream>

// Context::Context(std::vector<std::string> context) {
//     _name = context.at(0);
//     context.erase(context.begin());
//     std::cout << "CONTEXT NAME = " << _name << std::endl;
//     std::vector<std::string>::iterator it;
//     for (it = context.begin(); it != context.end(); it++) {

//         std::size_t	index = it.find('#');
//         if (index != std::string::npos) {  // ; can also be comments + need more checks with ' or " ???
//                 line = line.substr(0, index);
//         }
//         if (line.empty() || isOnlyWSpace(line)) {
//             std::cout << "THIS WAS ONLY A COMMENT OR AN EMPTY LINE" << std::endl;
//             continue;
//         }

//         std::cout << *it << std::endl;
//     }
// }

Context::Context(std::string name, std::string context): _name(name) {
    std::istringstream f(context);
	std::string line;
	std::string content;

    // std::cout << "CONTEXT NAME = " << _name << std::endl;
    while (getline(f, line)) {

        if (line.empty() || isOnlyWSpace(line)) {
            // std::cout << "HELLO" << std::endl;
            continue;
        }
        std::istringstream iss(line);
        std::string temp;
        line.clear();

        while (iss) {
            iss >> temp;
            line.append(temp);
            line.push_back(' ');
            temp.clear();
        }

        // remove comments

        std::size_t	index = line.find('#');
        if (index != std::string::npos) {  // ; can also be comments + need more checks with ' or " ???
                line = line.substr(0, index);
        }
        if (line.empty() || isOnlyWSpace(line)) {
            // std::cout << "THIS WAS ONLY A COMMENT OR AN EMPTY LINE" << std::endl;
            continue;
        }
        // check if context

        index = line.find('{');


        if (!content.empty() && index != 0) {
            // add to main directives
            // std::cout << "***** ADD MAIN DIRECTIVES *****" << std::endl;
            addDirective(content);
            content.clear();
            // continue;
        }

        content.append(line);


        // std::cout << "***** ADD NEW CONEXT *****" << std::endl;
        int open;
        if (index != std::string::npos) {
            open = 1;
            std::string name = line;
            std::string contextContent;
            while (getline(f, line)) { //what if many {} after each other or after ; not counted
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
            // if (open == 0)
            //     std::cout << "CONTEXT IS CLOSED" << std::endl;
            // else
            //     std::cout << "CONTEXT IS NOT CLOSED" << std::endl;

            // create context and send the remaining of the isstringstream
        }

        // std::cout << line << std::endl;
    }
    //where the } is to integrate into the last pair
    if (!content.empty()) {
        // std::cout << content << std::endl;
        addDirective(content);
        content.clear();
    }
    // std::cout << "\033[31m" << "#### DIR ####\n" << "\033[0m" << std::endl;
	// printMap();
}


Context::~Context() {}

bool Context::isOnlyWSpace(std::string line) const {
	size_t count = 0;

	for (size_t i = 0; i < line.length(); ++i) {
        if (isspace(line[i])) {
            count++;
        }
    }
	if (line.length() == count)
		return true;
	return false;
}

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

void Context::printMap() const {
	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;
	for (it = _directives.begin(); it != _directives.end(); ++it) {
		std::cout << it->first << ": ";

		std::vector<std::string>::const_iterator itv;
		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::cout << *itv << ", ";
		}
		std::cout << std::endl;
	}
}

void Context::printContent() const {
    std::cout << "CONTEXT = " << _name << std::endl;
    std::cout << "DIR" << std::endl;
    printMap();
    std::vector<Context>::const_iterator it;
	for (it = _context.begin(); it != _context.end(); it++) {
		it->printContent();
	}
}


std::string Context::getName(void) const {
    return (this->_name);
}

std::vector<Context>    Context::getContext(void) const {
    return (this->_context);
}

std::vector<std::pair<std::string, std::vector<std::string> > > Context::getDirectives(void) const {
    return (this->_directives);
}

