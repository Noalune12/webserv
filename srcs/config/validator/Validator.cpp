#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

#include "colors.h"
#include "rules.h"
#include "Validator.hpp"

// a mettre ailleurs ?
# define WEBSERV_PREFIX "webserv: "
# define EMERG "[emerg] "
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
	// validateGlobalDirective();
	// loop sur le vecteur de Context
	// keyNameCheck -> bracketCheck pour la directive context -> parameterCheck
	// puis meme chose que pour les globales directives mais a l'interieur du context.

	validateGlobalDirective();
	// printMap();
	// keyNameCheck();
	// clientMaxBodySize();
	// logger("test");
}


void	Validator::initAllowedContext(void) {

	_allowedInContext.push_back(std::make_pair(GLOBAL, std::vector<std::string>()));
	_allowedInContext.push_back(std::make_pair(SERV, std::vector<std::string>()));
	_allowedInContext.push_back(std::make_pair(LOCATION, std::vector<std::string>()));

	_allowedInContext[0].second.push_back(ERR_PAGE);
	_allowedInContext[0].second.push_back(ERR_LOG);
	_allowedInContext[0].second.push_back(CL_MAX_B_SYZE);

	_allowedInContext[1].second.push_back(LISTEN);
	_allowedInContext[1].second.push_back(SERV_NAME);
	_allowedInContext[1].second.push_back(ROOT);
	_allowedInContext[1].second.push_back(ERR_PAGE);
	_allowedInContext[1].second.push_back(CL_MAX_B_SYZE);
	_allowedInContext[1].second.push_back(INDEX);
	_allowedInContext[1].second.push_back(ALL_METHODS);
	_allowedInContext[1].second.push_back(AUTOINDEX);
	_allowedInContext[1].second.push_back(UPLOAD_TO);
	_allowedInContext[1].second.push_back(RETURN);

	_allowedInContext[2].second.push_back(ERR_PAGE);
	_allowedInContext[2].second.push_back(CL_MAX_B_SYZE);
	_allowedInContext[2].second.push_back(ROOT);
	_allowedInContext[2].second.push_back(INDEX);
	_allowedInContext[2].second.push_back(ALL_METHODS);
	_allowedInContext[2].second.push_back(AUTOINDEX);
	_allowedInContext[2].second.push_back(UPLOAD_TO);
	_allowedInContext[2].second.push_back(RETURN);
	_allowedInContext[2].second.push_back(ALIAS);
	_allowedInContext[2].second.push_back(CGI_PATH);
	_allowedInContext[2].second.push_back(CGI_EXT);

	// for (size_t i = 0; i < _allowedInContext.size(); ++i) {
	// 	std::cout << _allowedInContext[i].first << ": ";

	// 	for (size_t j = 0; j < _allowedInContext[i].second.size(); ++j) {
	// 		std::cout << _allowedInContext[i].second[j];
	// 		if (j != _allowedInContext[i].second.size() - 1)
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

	file << WEBSERV_PREFIX << EMERG << error << " in " << _config.getFilePath() << std::endl; // will add line of the misconfiguration once we switch from map to pair, will need another paramater with will be the line
	file << WEBSERV_PREFIX << "configuration file " << _config.getFilePath() << " test failed" << std::endl;

	file.close();
}


/* does the whole validation of global directives */
void	Validator::validateGlobalDirective(void) const {

	// check du nom de la directive -> done
	// check de la syntax des semicolons
	// check de la validite des parametres de la directives (propre a chaque directives)

	keyNameCheck(GLOBAL);
	const std::vector<std::pair<std::string, std::vector<std::string> > >& directives = _config.getGlobalDirective();

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;

	for (it = directives.begin(); it != directives.end(); ++it) {
		directiveCheck(it->first, it->second);
	}
}

void	Validator::directiveCheck(const std::string& directive, const std::vector<std::string>& values) const {

	(void) values;
	if (directive == ERR_PAGE) {
		validateErrorPage(values);
		// semicolonCheck()
	} else if (directive ==  ERR_LOG) {
		// semicolonCheck()
		// validateErrorLog()
	} else if (directive == CL_MAX_B_SYZE) {
		// not sure yet
	} else {
		// security but should no be required since we identified it in keyNameCheck()
		std::string errorMsg = "no validator found for directive \"" + directive + "\"";
		logger(errorMsg);
		throw std::runtime_error(errorMsg);
	}
}


void	Validator::keyNameCheck(const std::string& context) const {

	static const char	*directives[] = {
		ERR_PAGE, ERR_LOG, CL_MAX_B_SYZE, SERV, SERV_NAME, LISTEN, ROOT, INDEX,
		LOCATION, ALL_METHODS, AUTOINDEX, UPLOAD_TO, RETURN, ALIAS, CGI_PATH, CGI_EXT
	};

	const size_t	directivesCount = sizeof(directives) / sizeof(directives[0]);

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator	it;

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator contextIt;

	for (contextIt = _allowedInContext.begin(); contextIt != _allowedInContext.end(); ++contextIt) {
		if (contextIt->first == context) {
			break ;
		}
	}

	const std::vector<std::string>&	allowedDirectives = contextIt->second;

	const std::vector<std::pair<std::string, std::vector<std::string> > >& directivesToCheck = _config.getGlobalDirective();

	for (it = directivesToCheck.begin(); it != directivesToCheck.end(); ++it) {
		const std::string&	key = it->first;
		bool				found = false;

		std::vector<std::string>::const_iterator	allowedIt;
		for (allowedIt = allowedDirectives.begin(); allowedIt != allowedDirectives.end(); ++allowedIt) {
			if (key == *allowedIt) {
				found = true;
				// semicolonCheck(it->second, key); // not good maybe move elsewhere
				break ;
			}
		}
		if (!found) {
			for (size_t i = 0; i < directivesCount; ++i) {
			if (key == directives[i]) {
				std::string errorMsg = "\"" + key + "\" directive is not allowed here in ";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			}
		}
		std::string errorMsg = "unknown directive \"" + key + "\"";
		logger(errorMsg);
		throw std::invalid_argument(errorMsg);
		}
	}
}


// Maybe will had another layer of check depending on the directive name, some rules prevails over the semicolon check
void	Validator::semicolonCheck(const std::vector<std::string>& v, const std::string& directive) const {

	std::vector<std::string>::const_iterator itv;

	for (itv = v.begin(); itv != v.end(); ++itv) {
		const std::string&	value = *itv;

		if (value == " " || value.empty()) {
			continue ;
		}

		std::size_t	firstSemicolon = value.find(";");
		std::string	errorMsg;

		if (firstSemicolon != std::string::npos && firstSemicolon != value.length() - 1) {

			std::size_t nextNonSemicolon = value.find_first_not_of(";", firstSemicolon);

			if (nextNonSemicolon != std::string::npos && nextNonSemicolon < value.length()) {
				std::string	unknownPart = value.substr(firstSemicolon + 1);
				unknownPart = unknownPart.substr(0, unknownPart.find_first_of(";"));
				errorMsg = "unknown directive \"" + unknownPart + "\"";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			} else {
				errorMsg = "unexpected \";\"";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			}
		}

		if (firstSemicolon != value.length() - 1) {
			errorMsg = "directive \"" + directive + "\" is not terminated by \";\"";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}
	}
}


void	Validator::printMap() const {

	std::cout << GREEN "Validator:" RESET << std::endl;

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;

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

static bool	validateUnity(std::string leftover) {
	if (leftover.length() > 2)
		return (false);
	return (leftover[0] == 'k' || leftover[0] == 'K' || leftover[0] == 'm' || leftover[0] == 'M' || leftover[0] == 'g' || leftover[0] == 'G');
}

static bool	isWhitespace(char c) {
	return (std::isspace(static_cast<unsigned char>(c)));
}

// en attendant de changer pour une fonction du tableau de pointeurs sur fonctions: next prototype
// void	Validator::clientMaxBodySize(const std::vector<std::string>& values) const {
void	Validator::clientMaxBodySize(void) const {

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;

	for (it = _config.getGlobalDirective().begin(); it != _config.getGlobalDirective().end(); ++it) {

		std::vector<std::string>::const_iterator itv = it->second.end() - 1;

		std::istringstream	iss(*itv);
		int					value;
		std::string			leftover;

		if (!(iss >> value) || value <= 0) {
			std::string errorMsg = "invalid value \"" + *itv + "\" in \"" + it->first + "\" directive";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		} else {
			char nextChar = iss.peek();
			if (isWhitespace(nextChar)) {
				std::string errorMsg = "invalid number of arguments in \"" + it->first + "\" directive";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			} else if (!(iss >> leftover) && leftover.length() > 1) {
				std::string errorMsg = "invalid value \"" + *itv + "\" in \"" + it->first + "\" directive";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			} else if (!validateUnity(leftover)) {
				std::string errorMsg = "invalid value \"" + *itv + "\" in \"" + it->first + "\" directive";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			}
		}
	}
}

void	Validator::validateErrorPage(const std::vector<std::string>& v) const {

	static const int	error_codes[] = {
		301, 302, 303, 307, 308,
		400, 403, 404, 405, 408, 429,
		500, 505
	};

	const size_t	validCodeCount = sizeof(error_codes) / sizeof(error_codes[0]);

	std::vector<std::string>::const_iterator itv;

	// need to check there is at least 2 elements, one error code and one path

	// v.end() - 1 feels wrong but I dont know how to check it otherwise. (it is wrong, will check later)
	for (itv = v.begin(); itv != v.end() - 1; ++itv) {
		std::istringstream iss(*itv);
		int value;

		if (!(iss >> value)) {
			std::string errorMsg = "invalid value \"" + *itv + "\"";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}

		int	nextChar = iss.peek();
		if (nextChar != EOF) {
			std::string errorMsg = "invalid value \"" + *itv + "\"";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}

		if (!(value >= 300 && value <= 599)) {
			std::string errorMsg = "value \"" + *itv + "\" must be between 300 and 599";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}


		bool	validCode = false;
		for (size_t i = 0; i < validCodeCount; ++i) {
			if (error_codes[i] == value) {
				validCode = true;
				break ;
			}
		}

		if (!validCode) {
			std::ostringstream	oss;
			oss << value;
			std::string errorMsg = "invalid value \"" + oss.str() + "\"";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}
	}
}
