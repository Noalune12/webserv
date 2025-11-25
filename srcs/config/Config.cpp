#include <iostream>
#include <sstream>
#include <algorithm>

#include "Config.hpp"
#include "FileReader.hpp"

// Config::Config() {}

Config::Config(const std::string& configFile /* nom a revoir j'ai mis autre chose dans mes fichiers de test */) {

	try {

		FileReader reader(configFile);

		_filePath = reader.getFilePath();
		_fileContent = reader.getFileContent();

		// std::cout << "\033[31m" << "#### FILE CONTENT ####\n" << "\033[0m" << _fileContent << std::endl;

		std::istringstream f(_fileContent);
		std::string line;

		while (getline(f, line)) {

			// remove comments

			std::size_t	index = line.find('#');
			if (index != std::string::npos) {  // ; can also be comments + need more checks with ' or " ???
					line = line.substr(0, index);
			}
			if (line.empty() || isOnlyWSpace(line)) {
				std::cout << "THIS WAS ONLY A COMMENT OR AN EMPTY LINE" << std::endl;
				continue;
			}

			// check if context

			index = line.find('{');
			int open;
			if (index != std::string::npos) {
				open = 1;
				while (getline(f, line)) {
					if (line.find('{') != std::string::npos)
						open++;
					else if (line.find('}') != std::string::npos)
						open--;
					if (open == 0)
						break;
					std::cout << line << "~~~" << std::endl;
				}
				// create context and send the remaining of the isstringstream
			} else {
				// add to main directives
				std::cout << "***** ADD MAIN DIRECTIVES *****" << std::endl;
				addDirective(line);
			}
			// std::cout << line << "~~~" << std::endl;
		}
		std::cout << "\033[31m" << "#### GLOBAL DIR ####\n" << "\033[0m" << std::endl;
		printMap();
	} catch(const std::exception& e) {
		std::cerr << "Server initialization failed: " << e.what() << std::endl;
		throw;
	}
}

Config::~Config() {}

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


bool isNotWhitespace(char c) {
    return !std::isspace(static_cast<unsigned char>(c));
}

bool isWhitespace(char c) {
    return std::isspace(static_cast<unsigned char>(c));
}

void Config::addDirective(std::string line) {

	// remove whitespaces
	std::string::iterator it = std::find_if(line.begin(), line.end(), isNotWhitespace);
	int pos = std::distance(line.begin(), it);
	line = line.substr(pos, line.length());

	std::string dir;
	std::vector<std::string> arg;

	// get directives
	it = std::find_if(line.begin(), line.end(), isWhitespace);
	if (it == line.end()) {
		dir = line.substr(0, line.length());
		_globalDirectives[dir] = arg;
		return;
	}
	pos = std::distance(line.begin(), it);
	dir = line.substr(0, pos);
	line = line.substr(pos + 1, line.length());
	// std::size_t index = line.find(' '); // what about other whitespaces
	// if (index == std::string::npos) {
	// 	dir = line.substr(0, line.length());
	// 	_globalDirectives[dir] = arg;
	// 	return;
	// } else {
	// 	dir = line.substr(0, index); // add empty vector ?
	// 	line = line.substr(index + 1, line.length());
	// }
	// std::cout << dir << "~~~" << std::endl;

	// get directives arguments
	while (!line.empty()) {
		// remove whitespaces
		it = std::find_if(line.begin(), line.end(), isNotWhitespace);
		pos = std::distance(line.begin(), it);
		line = line.substr(pos, line.length());

		it = std::find_if(line.begin(), line.end(), isWhitespace);
		if (it == line.end() && !isOnlyWSpace(line)) {
			arg.push_back(line.substr(0, line.length()));
			break;
		} else if (it != line.end()) {
			pos = std::distance(line.begin(), it);
			arg.push_back(line.substr(0, pos));
			line = line.substr(pos, line.length());
		} else {
			break;
		}
	}

	// std::cout << "LINE AFTER DIR EXTRACTED = " << line << std::endl;
	// while((index = line.find(' ')) != std::string::npos) {
	// 	arg.push_back(line.substr(0, index));
	// 	line = line.substr(index + 1, line.length());
	// 	std::cout << index << std::endl;
	// 	std::cout << line << std::endl;
	// }
	// std::cout << line << std::endl;
	// if (!line.empty() || !isOnlyWSpace(line))
	// 	arg.push_back(line.substr(0, line.length()));
	//remove white spaces at the beginning

	// IF ALREADY IN MAP -> push back to the vector + separate with a space
	std::map<std::string, std::vector<std::string> >::const_iterator itm;
	for (itm = _globalDirectives.begin(); itm != _globalDirectives.end(); ++itm) {
		if (itm->first == dir) {
			std::vector<std::string> newArg = itm->second;
			newArg.push_back(" ");
			std::vector<std::string>::const_iterator itv;
			for (itv = arg.begin(); itv != arg.end(); ++itv) {
				newArg.push_back(*itv);
			}
			arg = newArg;
			break;
		}
	}
	_globalDirectives[dir] = arg;
}

void Config::printMap() const {
	std::map<std::string, std::vector<std::string> >::const_iterator it;
	for (it = _globalDirectives.begin(); it != _globalDirectives.end(); ++it) {
		std::cout << it->first << ": ";
		
		std::vector<std::string>::const_iterator itv;
		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::cout << *itv << ", ";
		}
		std::cout << std::endl;
	}
}

// void Config::removeWSpaces() {
// 		for (size_t i = 0; i < line.length(); ++i) {
//         if (isspace(line[i])) {
//             count++;
//         }
//     }
// }