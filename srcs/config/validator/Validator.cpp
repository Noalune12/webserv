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

# define LOG_FILE "var/log/error.log"

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

	_allowedInContext[GLOBAL_VALUE].second.push_back(ERR_PAGE);
	_allowedInContext[GLOBAL_VALUE].second.push_back(CL_MAX_B_SYZE);

	_allowedInContext[SERV_VALUE].second.push_back(LISTEN);
	_allowedInContext[SERV_VALUE].second.push_back(SERV_NAME);
	_allowedInContext[SERV_VALUE].second.push_back(ROOT);
	_allowedInContext[SERV_VALUE].second.push_back(ERR_PAGE);
	_allowedInContext[SERV_VALUE].second.push_back(CL_MAX_B_SYZE);
	_allowedInContext[SERV_VALUE].second.push_back(INDEX);
	_allowedInContext[SERV_VALUE].second.push_back(ALL_METHODS);
	_allowedInContext[SERV_VALUE].second.push_back(AUTOINDEX);
	_allowedInContext[SERV_VALUE].second.push_back(UPLOAD_TO);
	_allowedInContext[SERV_VALUE].second.push_back(RETURN);

	_allowedInContext[LOCATION_VALUE].second.push_back(ERR_PAGE);
	_allowedInContext[LOCATION_VALUE].second.push_back(CL_MAX_B_SYZE);
	_allowedInContext[LOCATION_VALUE].second.push_back(ROOT);
	_allowedInContext[LOCATION_VALUE].second.push_back(INDEX);
	_allowedInContext[LOCATION_VALUE].second.push_back(ALL_METHODS);
	_allowedInContext[LOCATION_VALUE].second.push_back(AUTOINDEX);
	_allowedInContext[LOCATION_VALUE].second.push_back(UPLOAD_TO);
	_allowedInContext[LOCATION_VALUE].second.push_back(RETURN);
	_allowedInContext[LOCATION_VALUE].second.push_back(ALIAS);
	_allowedInContext[LOCATION_VALUE].second.push_back(CGI_PATH);
	_allowedInContext[LOCATION_VALUE].second.push_back(CGI_EXT);

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

	_directiveValidators[CL_MAX_B_SYZE] = &Validator::validateClientMaxBodySize;
	_directiveValidators[ERR_PAGE] = &Validator::validateErrorPage;
}

void	Validator::logger(const std::string& error) const {

	static const char	*outputFile = LOG_FILE;
	std::ofstream		file;

	file.open(outputFile, std::ios::out | std::ios::app);

	file << WEBSERV_PREFIX << EMERG << error << " in " << _config.getFilePath() << std::endl; // will NOT add line of the misconfiguration
	file << WEBSERV_PREFIX << "configuration file " << _config.getFilePath() << " test failed" << std::endl;

	file.close();
}


void	Validator::validateGlobalDirective(void) const {

	keyNameCheck(GLOBAL);

	const std::vector<std::pair<std::string, std::vector<std::string> > >& directives = _config.getGlobalDirective();
	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;

	for (it = directives.begin(); it != directives.end(); ++it) {
		std::map<std::string, DirectiveValidator>::const_iterator validatorIt = _directiveValidators.find(it->first);
		if (validatorIt != _directiveValidators.end()) {
			(this->*(validatorIt->second))(it->second);
		}
	}

	/* temp, debug */
	std::cout << BLUE "working properly for global directives" << RESET << std::endl;
}

void	Validator::keyNameCheck(const std::string& context) const {

	static const char	*directives[] = {
		ERR_PAGE, CL_MAX_B_SYZE, SERV, SERV_NAME, LISTEN, ROOT, INDEX,
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

void	Validator::semicolonCheck(const std::vector<std::string>& v, const std::string& directive) const {

	std::vector<std::vector<std::string> >					groups = splitDirectiveGroups(v);
	std::vector<std::vector<std::string> >::const_iterator	groupIt;

	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;
		const std::string&				lastValue = group.back();
		std::string						errorMsg;

		if (lastValue[lastValue.length() - 1] != ';') {
			errorMsg = "directive \"" + directive + "\" is not terminated by \";\"";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}

		std::size_t firstSemicolon = lastValue.find(';');

		if (firstSemicolon != lastValue.length() - 1) {

			std::size_t	nextNonSemicolon = lastValue.find_first_not_of(";", firstSemicolon);

			if (nextNonSemicolon != std::string::npos) {
				std::string	unknownPart = lastValue.substr(firstSemicolon + 1);
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

static bool	validateUnity(const std::string& leftover) {
	if (leftover.length() != 1)
		return (false);
	return (leftover[0] == 'k' || leftover[0] == 'K' || leftover[0] == 'm' || leftover[0] == 'M' || leftover[0] == 'g' || leftover[0] == 'G');
}

void	Validator::validateClientMaxBodySize(const std::vector<std::string>& values) const {

	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values);

	std::vector<std::vector<std::string> >::const_iterator	groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		size_t		argCount = 0;
		std::string	value;

		std::vector<std::string>::const_iterator	it;
		for (it = group.begin(); it != group.end(); ++it) {
			if (!it->empty() && (*it)[0] != ';') {
				++argCount;
				if (value.empty()) {
					value = *it;
				}
			}
		}

		if (argCount != 1) {
			std::string	errorMsg = "invalid number of arguments in \"client_max_body_size\" directive";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}

		std::string	cleanValue = value;
		while (!cleanValue.empty() && cleanValue[cleanValue.length() - 1] == ';') {
			cleanValue = cleanValue.substr(0, cleanValue.length() - 1);
		}

		std::istringstream	iss(cleanValue);
		long				number;

		if (!(iss >> number)) {
			std::string	errorMsg = "\"client_max_body_size\" directive invalid value";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}

		if (number <= 0) {
			std::string	errorMsg = "\"client_max_body_size\" directive invalid value";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}

		std::string	unit;
		iss >> unit;

		if (!validateUnity(unit)) {
			std::string errorMsg = "\"client_max_body_size\" directive invalid value";
			logger(errorMsg);
			throw std::invalid_argument(errorMsg);
		}
	}

	if (groups.size() > 1) {
		std::string errorMsg = "\"client_max_body_size\" directive is duplicate";
		logger(errorMsg);
		throw std::invalid_argument(errorMsg);
	}
	semicolonCheck(values, CL_MAX_B_SYZE);
}

void	Validator::validateErrorPage(const std::vector<std::string>& values) const {

	static const int	error_codes[] = {
		301, 302, 303, 307, 308,
		400, 403, 404, 405, 408, 429,
		500, 505
	};

	const size_t	validCodeCount = sizeof(error_codes) / sizeof(error_codes[0]);

	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values);

	std::vector<std::vector<std::string> >::const_iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateMinimumArgs(group, 2, ERR_PAGE);

		for (size_t i = 0; i < group.size() - 1; ++i) {

			std::istringstream	iss(group[i]);
			int					value;

			if (!(iss >> value)) {
				std::string	errorMsg = "invalid value \"" + group[i] + "\"";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			}

			if (iss.peek() != EOF) {
				std::string	errorMsg = "invalid value \"" + group[i] + "\"";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			}

			if (!(value >= 300 && value <= 599)) {
				std::ostringstream	oss;
				oss << value;
				std::string	errorMsg = "value \"" + oss.str() + "\" must be between 300 and 599";
				logger(errorMsg);
				throw std::invalid_argument(errorMsg);
			}

			bool	validCode = false;
			for (size_t j = 0; j < validCodeCount; ++j) {
				if (error_codes[j] == value) {
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
	semicolonCheck(values, ERR_PAGE);
}

std::vector<std::vector<std::string> >	Validator::splitDirectiveGroups(const std::vector<std::string>& values) const {

	std::vector<std::vector<std::string> >	groups;
	std::vector<std::string>				current;

	std::vector<std::string>::const_iterator it;

	for (it = values.begin(); it != values.end(); ++it) {
		if (*it == " ") {
			if (!current.empty()) {
				groups.push_back(current);
				current.clear();
			}
		} else {
			current.push_back(*it);
		}
	}
	if (!current.empty()) {
		groups.push_back(current);
	}
	return (groups);
}

void	Validator::validateMinimumArgs(const std::vector<std::string>& group, size_t minArgs, const std::string& directive) const {

	size_t validArgsCount = 0;

	std::vector<std::string>::const_iterator	it;
	for (it = group.begin(); it != group.end(); ++it) {
		if (!it->empty() && (*it)[0] != ';') {
			++validArgsCount;
		}
	}

	if (validArgsCount < minArgs) {
		std::string	errorMsg = "invalid number of arguments in \"" + directive + "\" directive";
		logger(errorMsg);
		throw std::invalid_argument(errorMsg);
	}
}

// debug
void	Validator::printGroups(const std::vector<std::vector<std::string> >& groups) const {

	std::cout << "Groups count: " << groups.size() << std::endl;

	std::vector<std::vector<std::string> >::const_iterator	groupIt;
	size_t													groupIndex = 0;

	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		std::cout << "  Group[" << groupIndex << "]: ";

		std::vector<std::string>::const_iterator	it;
		for (it = groupIt->begin(); it != groupIt->end(); ++it) {
			std::cout << "\"" << *it << "\"";
			if (it + 1 != groupIt->end())
				std::cout << ", ";
		}
		std::cout << std::endl;
		++groupIndex;
	}
}
