#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "colors.h"
#include "FileReader.hpp"
#include "Validator.hpp"

// Config::Config() {}

Config::Config(const std::string& configFile /* nom a revoir j'ai mis autre chose dans mes fichiers de test */) : _filePath(configFile), _fileContent() {

	try {

		FileReader reader(_filePath);

		_fileContent = reader.getFileContent();

		std::istringstream f(_fileContent);
		std::string line;
		std::string content;

		while (getline(f, line)) {

			// std::cout << "START" << std::endl;

			if (line.empty() || isOnlyWSpace(line)) {
				// std::cout << "HELLO" << std::endl;
				continue;
			}

			std::istringstream	iss(line);
			std::string temp;
			line.clear();

			while (iss) {
				iss >> temp;
				if (temp.empty())
					break;
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

			index = line.find('{');
			// std::cout << index << std::endl;
			if (!content.empty() && index != 0) {
				// add to main directives
				// std::cout << "***** ADD MAIN DIRECTIVES ***** with " << content << std::endl;
				addDirective(content);
				content.clear();
				// continue;
			}


			content.append(line);
			// line.clear();
			// std::cout << "LINE = " << line << "      CONTENT = " << content << "~~~~" << std::endl;
			// check if context

			// std::cout << "***** ADD NEW CONTEXT *****" << std::endl;
			int open;
			if (index != std::string::npos) {

				// std::size_t nextOpen;
				// std::size_t nextClose;
				std::string name = content;
				open = 1;

				// // check closed and opened in "name"
				// index = content.find('{');
				// nextOpen = content.find('{', index+1);
				// nextClose = content.find('}', index+1);
				// while (nextOpen != std::string::npos || nextClose != std::string::npos) {
				// 	std::cout << "next open = " << nextOpen << "\nnext close = " << nextClose << std::endl;
				// 	if (nextOpen < nextClose) {
				// 		std::cout << "one is open" << std::endl;
				// 		open++;
				// 		index = nextOpen;
				// 		nextOpen = content.find('{', index+1);
				// 	} else {
				// 		std::cout << "one is close" << std::endl;
				// 		open--;
				// 		index = nextClose;
				// 		nextClose = content.find('}', index+1);
				// 	}
				// }
				// std::cout << "next open = " << nextOpen << "\nnext close = " << nextClose << std::endl;
				// std::cout << "OPEN = " << open << std::endl;

				std::string contextContent;
				while (getline(f, line)) { //what if many {} after each other or after ; not counted



					// check where the closing one is
					// append while not finding it
					// if closing is inside a line -> cut the line and add the remaining to the a main directives
					// if found set the boolean to true




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
					// std::cout << "CONTEXT IS CLOSED" << std::endl;
				// else
					// std::cout << "CONTEXT IS NOT CLOSED" << std::endl;

				// create context and send the remaining of the isstringstream
			}
			// std::cout << line << "~~~" << std::endl;
		}
		if (!content.empty()) {
			addDirective(content); // temporary oneline-file fix
			content.clear();
		}
		// std::cout << "\033[31m" << "#### GLOBAL DIR ####\n" << "\033[0m" << std::endl;
		// printContent();


		// std::cout << _filePath << std::endl;
		// std::cout << _fileContent << std::endl;

		Validator	validator(*this);
		validator.validate();

	} catch(const std::exception& e) {
		std::cerr << "Server initialization failed: " << e.what() << std::endl;
		throw;
	}
}

Config::~Config() {}

Config&	Config::getConfig(void) {
	return (*this);
}

bool Config::isOnlyWSpace(std::string line) const {
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


// bool isNotWhitespace(char c) {
//     return !std::isspace(static_cast<unsigned char>(c));
// }

// bool isWhitespace(char c) {
//     return std::isspace(static_cast<unsigned char>(c));
// }

// void Config::addDirective(std::string line) {

// 	// remove whitespaces
// 	std::string::iterator it = std::find_if(line.begin(), line.end(), isNotWhitespace);
// 	int pos = std::distance(line.begin(), it);
// 	line = line.substr(pos, line.length());

// 	std::string dir;
// 	std::vector<std::string> arg;

// 	// get directives
// 	it = std::find_if(line.begin(), line.end(), isWhitespace);
// 	if (it == line.end()) {
// 		dir = line.substr(0, line.length());
// 		_globalDirectives[dir] = arg;
// 		return;
// 	}
// 	pos = std::distance(line.begin(), it);
// 	dir = line.substr(0, pos);
// 	line = line.substr(pos + 1, line.length());
// 	// std::size_t index = line.find(' '); // what about other whitespaces
// 	// if (index == std::string::npos) {
// 	// 	dir = line.substr(0, line.length());
// 	// 	_globalDirectives[dir] = arg;
// 	// 	return;
// 	// } else {
// 	// 	dir = line.substr(0, index); // add empty vector ?
// 	// 	line = line.substr(index + 1, line.length());
// 	// }
// 	// std::cout << dir << "~~~" << std::endl;

// 	// get directives arguments
// 	while (!line.empty()) {
// 		// remove whitespaces
// 		it = std::find_if(line.begin(), line.end(), isNotWhitespace);
// 		pos = std::distance(line.begin(), it);
// 		line = line.substr(pos, line.length());

// 		it = std::find_if(line.begin(), line.end(), isWhitespace);
// 		if (it == line.end() && !isOnlyWSpace(line)) {
// 			arg.push_back(line.substr(0, line.length()));
// 			break;
// 		} else if (it != line.end()) {
// 			pos = std::distance(line.begin(), it);
// 			arg.push_back(line.substr(0, pos));
// 			line = line.substr(pos, line.length());
// 		} else {
// 			break;
// 		}
// 	}

// 	// std::cout << "LINE AFTER DIR EXTRACTED = " << line << std::endl;
// 	// while((index = line.find(' ')) != std::string::npos) {
// 	// 	arg.push_back(line.substr(0, index));
// 	// 	line = line.substr(index + 1, line.length());
// 	// 	std::cout << index << std::endl;
// 	// 	std::cout << line << std::endl;
// 	// }
// 	// std::cout << line << std::endl;
// 	// if (!line.empty() || !isOnlyWSpace(line))
// 	// 	arg.push_back(line.substr(0, line.length()));
// 	//remove white spaces at the beginning

// 	// IF ALREADY IN MAP -> push back to the vector + separate with a space
// 	std::map<std::string, std::vector<std::string> >::const_iterator itm;
// 	for (itm = _globalDirectives.begin(); itm != _globalDirectives.end(); ++itm) {
// 		if (itm->first == dir) {
// 			std::vector<std::string> newArg = itm->second;
// 			newArg.push_back(" ");
// 			std::vector<std::string>::const_iterator itv;
// 			for (itv = arg.begin(); itv != arg.end(); ++itv) {
// 				newArg.push_back(*itv);
// 			}
// 			arg = newArg;
// 			break;
// 		}
// 	}
// 	_globalDirectives[dir] = arg;
// }

void Config::addDirective(std::string line) {

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
	// std::vector<std::pair<std::string, std::vector<std::string> > >::iterator itm = _globalDirectives.find(dir);

	std::vector<std::pair<std::string, std::vector<std::string> > >::iterator itm = _globalDirectives.begin();
	for (; itm != _globalDirectives.end(); itm++) {
		if (itm->first == dir)
			break ;
	}


	if (itm != _globalDirectives.end()) {
		// Directive already exists - append new arguments with separator
		// I use second here to interact directly in the string vector instead of declaring new memory
		itm->second.push_back(" "); // Keeping the separation, might not be needed anympre
		itm->second.insert(itm->second.end(), args.begin(), args.end());
	} else {
		// New directive
		_globalDirectives.push_back(std::make_pair(dir, args));
		// _globalDirectives[dir] = args;
	}
}

void Config::printMap() const {
	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;
	for (it = _globalDirectives.begin(); it != _globalDirectives.end(); ++it) {
		std::cout << it->first << ": ";

		std::vector<std::string>::const_iterator itv;
		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::cout << *itv << ", ";
		}
		std::cout << std::endl;
	}
}


/* GETTERS */

const std::string&	Config::getFilePath(void) const {
	return (this->_filePath);
}

std::string			Config::getFileContent(void) const {
	return (this->_fileContent);
}

std::vector<std::pair<std::string, std::vector<std::string> > >&	Config::getGlobalDirective(void) {
	return (this->_globalDirectives);
}

std::vector<Context>	Config::getVectorContext(void) const {
	return (this->_context);
}


/* TEMP */

// void Config::setUpTest(void) {

// 	/* client_max_body_size */
// 	// std::vector<std::string> cl_max_b_size;

// 	// cl_max_b_size.push_back("10m;");


// 	// /* error_page */
// 	// std::vector<std::string> error_page;

// 	// error_page.push_back("404");
// 	// error_page.push_back("./test-path-/404-test.html;");


// 	// /* globalDirectives temporary tester */

// 	// _globalDirectives.push_back(std::make_pair("client_max_body_size", cl_max_b_size));
// 	// _globalDirectives.push_back(std::make_pair("error_page", error_page));
// }

void	Config::printContent() const {
	std::cout << "MAIN DIR" << std::endl;
	printMap();
	std::vector<Context>::const_iterator it;
	for (it = _context.begin(); it != _context.end(); it++) {
		it->printContent();
	}
}
