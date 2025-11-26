#include <iostream>
#include <sstream>
#include <exception>

#include "rules.h"
#include "colors.h"
#include "Validator.hpp"

// #include "Tokenizer.hpp"


Validator::Validator() : _globalDirectives() {
	// _configData = config.getConfig(config);
}

Validator::~Validator() {}




void	Validator::printMap() const {

	std::cout << GREEN "Validator:" RESET << std::endl;

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

static bool	validateUnity(char leftover) {
	return (leftover == 'k' || leftover == 'K' || leftover == 'm' || leftover == 'M' || leftover == 'g' || leftover == 'G');
}

static bool	isWhitespace(char c) {
    return (std::isspace(static_cast<unsigned char>(c)));
}

void	Validator::clientMaxBodySize(void) const {

	std::map<std::string, std::vector<std::string> >::const_iterator it;

	for (it = _globalDirectives.begin(); it != _globalDirectives.end(); ++it) {

		std::vector<std::string>::const_iterator itv;

		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::istringstream	iss(*itv);
			int					value;
			char				leftover;

			if (!(iss >> value) || value <= 0) {
				throw std::invalid_argument("Error in key: " + it->first + *itv + " cannot be negative");
				// std::cout << "Error in key '" + it->first + "': '" + *itv + "' cannot be negative" << std::endl;
			} else {
				char nextChar = iss.peek();
				if (isWhitespace(nextChar)) {
					throw std::invalid_argument("Error in key: " + it->first + *itv + " has whitespaces");
					// std::cout << "Error in key '" + it->first + "': '" + *itv + "' has whitespaces" << std::endl;
				} else if (!(iss >> leftover)) {
					throw std::invalid_argument("Error in key: " + it->first + " " + *itv + " has no unity");
					// std::cout << "Error in key '" + it->first + "': '" + *itv + "' has no unity" << std::endl;
				} else if (!validateUnity(leftover)) {
					throw std::invalid_argument("Error in key: " + it->first + *itv + " has a wrong unity");
					// std::cout << "Error in key '" + it->first + "': '" + *itv + "' has a wrong unity" << std::endl;
				}
			}
		}
	}
}
