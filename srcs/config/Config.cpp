#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "Config.hpp"
#include "FileReader.hpp"
#include "Utils.hpp"


Config::Config(const std::string& configFile /* nom a revoir j'ai mis autre chose dans mes fichiers de test */) {

	try {

		FileReader reader(configFile);

		_filePath = reader.getFilePath();
		_fileContent = reader.getFileContent();

		std::istringstream f(_fileContent);
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

		printContent();
	} catch(const std::exception& e) {
		std::cerr << "Server initialization failed: " << e.what() << std::endl;
		throw;
	}
}

Config::~Config() {}


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
    } else
		_globalDirectives.push_back(std::make_pair(dir, args));
}

void Config::printContent() const {
	std::cout << "MAIN DIR" << std::endl;
	Utils::printDirectives(_globalDirectives);
	std::cout << std::endl;
	std::vector<Context>::const_iterator it;
	for (it = _context.begin(); it != _context.end(); it++) {
		it->printContext();
	}
}