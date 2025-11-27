#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

#include "colors.h"
#include "rules.h"
#include "Validator.hpp"

// a mettre ailleurs ?
# define WEBSERV_ERROR_HEADER "webserv: "
# define EMERGE "[emerge] "
# define UNKNOWN_DIR "unknown directive "
# define UNEXPECTED "unexpected "
# define CONF_FILE "configuration file "
# define TEST_FAILED "test failed\n"

Validator::Validator(Config& config) : _config(config), _allowedInContext() {
	initAllowedContext();
	initValidators();
}

Validator::~Validator() {}

void	Validator::validate(void) {

	// validation par contexte -> creer des regles de validation en fonction du contexte.

	// loop sur globalDirectives -> je sais que ce sont des directives dont je peux faire:
	// keyNameCheck -> semiColonCheck -> parameterCheck en fonctions de la directive. (tableaux de pointeurs sur fonctions ?)

	// loop sur le vecteur de Context
	// keyNameCheck -> bracketCheck pour la directive context -> parameterCheck
	// puis meme chose que pour les globales directives mais a l'interieur du context.

	printMap();
	keyNameCheck();
	logger("test");
}

void	Validator::initAllowedContext(void) {

	// Global Context
	_allowedInContext[GLOBAL].push_back(ERR_PAGE);
	_allowedInContext[GLOBAL].push_back(ERR_LOG);
	_allowedInContext[GLOBAL].push_back(CL_MAX_B_SYZE);
	_allowedInContext[GLOBAL].push_back(SERV); //  a remove ? je sais pas encore si on considère les contexts comme des directives

	// Server Context
	_allowedInContext[SERV].push_back(LISTEN);
	_allowedInContext[SERV].push_back(SERV_NAME);
	_allowedInContext[SERV].push_back(ROOT);
	_allowedInContext[SERV].push_back(ERR_PAGE);
	_allowedInContext[SERV].push_back(CL_MAX_B_SYZE);
	_allowedInContext[SERV].push_back(INDEX);
	_allowedInContext[SERV].push_back(LOCATION); // same
	_allowedInContext[SERV].push_back(ALL_METHODS);
	_allowedInContext[SERV].push_back(AUTOINDEX);
	_allowedInContext[SERV].push_back(UPLOAD_TO);
	_allowedInContext[SERV].push_back(RETURN);

	// Location Context
	_allowedInContext[LOCATION].push_back(ERR_PAGE);
	_allowedInContext[LOCATION].push_back(CL_MAX_B_SYZE);
	_allowedInContext[LOCATION].push_back(ROOT);
	_allowedInContext[LOCATION].push_back(INDEX);
	_allowedInContext[LOCATION].push_back(ALL_METHODS);
	_allowedInContext[LOCATION].push_back(AUTOINDEX);
	_allowedInContext[LOCATION].push_back(UPLOAD_TO);
	_allowedInContext[LOCATION].push_back(RETURN);
	_allowedInContext[LOCATION].push_back(ALIAS);
	_allowedInContext[LOCATION].push_back(CGI_PATH);
	_allowedInContext[LOCATION].push_back(CGI_EXT);

	// std::map<std::string, std::vector<std::string> >::const_iterator it;

	// for (it = _allowedInContext.begin(); it != _allowedInContext.end(); ++it) {
	// 	std::cout << it->first << ": ";

	// 	std::vector<std::string>::const_iterator itv;
	// 	for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
	// 		std::cout << *itv;
	// 		if (itv != it->second.end() - 1)
	// 			std::cout << ", ";
	// 	}
	// 	std::cout << std::endl;
	// }
}

void	Validator::initValidators(void) {

	// _directiveValidators[CL_MAX_B_SYZE] = &Validator::clientMaxBodySize;
}

void	Validator::logger(const std::string& error) const {

	static const char *outputFile = "var/log/error.log";
	std::ofstream	file;

	file.open(outputFile, std::ios::out | std::ios::app);
	file << WEBSERV_ERROR_HEADER << error << "\n";
	file << WEBSERV_ERROR_HEADER << CONF_FILE << "./" << _config.getFilePath() + " " << TEST_FAILED;
	file.close();
}

void	Validator::keyNameCheck(void) const {

	const char	*directives[] = {
		ERR_PAGE, ERR_LOG, CL_MAX_B_SYZE, SERV, SERV_NAME, LISTEN, ROOT, INDEX,
		LOCATION, ALL_METHODS, AUTOINDEX, UPLOAD_TO, RETURN, ALIAS, CGI_PATH, CGI_EXT
	};

	std::map<std::string, std::vector<std::string> >::const_iterator	it;

	const size_t	directivesCount = sizeof(directives) / sizeof(directives[0]);

	for (it = _config.getGlobalDirective().begin() ; it != _config.getGlobalDirective().end(); ++it) {
		const std::string&	key = it->first;
		bool				found = false;
		for (size_t i = 0; i < directivesCount; ++i) {
			if (key == directives[i]) {
				std::cout << key << std::endl;
				found = true;
				semicolonCheck(it->second);
				// check directives settings via table of function pointer
				break ;
			}
		}
		if (!found) {
			throw std::invalid_argument("Error in key: '" + it->first +  "' is an invalid directive");
		}
	}
}

void	Validator::semicolonCheck(const std::vector<std::string>& v) const {

	std::vector<std::string>::const_iterator itv;

    for (itv = v.begin(); itv != v.end(); ++itv) {
        const std::string&	value = *itv;

        if (value == " ") {
            std::cout << YELLOW << "skipping space" << RESET << std::endl;
            continue ;
        }

        std::size_t	found = value.find(";");
        std::cout << "value: '" << value << "'" << std::endl << "found: " << found << std::endl << "length: " << value.length() - 1 << std::endl;

        if (found != std::string::npos && found != value.length() - 1) {
            std::cout << RED << "Semicolon in wrong position in value: '" << value << "'" << RESET << std::endl;
            throw std::invalid_argument("Error in value: '" + value + "' ");
        }

        if (itv == v.end() - 1) {
            if (found == value.length() - 1) {
                std::cout << GREEN << "last char of last element of vector is semicolon: " << value << RESET << std::endl;
                return;
            } else {
                std::cout << RED << "Last element missing semicolon: " << value << RESET << std::endl;
                throw std::invalid_argument("Last element must end with semicolon: '" + value + "' ");
            }
        }
    }
}


void	Validator::printMap() const {

	std::cout << GREEN "Validator:" RESET << std::endl;

	std::map<std::string, std::vector<std::string> >::const_iterator it;

	for (it = _config.getGlobalDirective().begin(); it != _config.getGlobalDirective().end(); ++it) {
		std::cout << it->first << ": ";

		std::vector<std::string>::const_iterator itv;
		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::cout << *itv;
			if (itv != it->second.end() - 1)
				std::cout << ", ";
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

// en attendant de changer pour une fonction du tableau de pointeurs sur fonctions: next prototype
// void	Validator::clientMaxBodySize(const std::vector<std::string>& values) const {
void	Validator::clientMaxBodySize(void) const {

	std::map<std::string, std::vector<std::string> >::const_iterator it;

	for (it = _config.getGlobalDirective().begin(); it != _config.getGlobalDirective().end(); ++it) {

		std::vector<std::string>::const_iterator itv;

		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::istringstream	iss(*itv);
			int					value;
			char				leftover;

			if (!(iss >> value) || value <= 0) {
				throw std::invalid_argument("Error in key: " + it->first + *itv + " cannot be negative");
			} else {
				char nextChar = iss.peek();
				if (isWhitespace(nextChar)) {
					throw std::invalid_argument("Error in key: " + it->first + *itv + " has whitespaces");
				} else if (!(iss >> leftover)) {
					throw std::invalid_argument("Error in key: " + it->first + " " + *itv + " has no unity");
				} else if (!validateUnity(leftover)) {
					throw std::invalid_argument("Error in key: " + it->first + *itv + " has a wrong unity");
				}
			}
		}
	}
}
