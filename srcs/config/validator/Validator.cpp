#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

#include "colors.h"
#include "rules.h"
#include "Validator.hpp"
#include "Utils.hpp"

Validator::Validator(Config& config) : _config(config), _currentContext(NULL), _allowedInContext() {
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
	validateServerContexts();

	// we have to check listen and server_name between Contexts to avoid:
	// nginx: [warn] conflicting server name "localhost" on 0.0.0.0:80, ignored

	// server {
	// 	listen 80;
	// 	server_name localhost;
	// }

	// server {
	// 	listen 80;
	// 	server_name localhost;
	// }
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



void	Validator::validateGlobalDirective(void) const {

	const std::vector<std::pair<std::string, std::vector<std::string> > >& directives = _config.getTokenizer().getGlobalDirective();

	keyNameCheck(directives, GLOBAL_VALUE);

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		std::map<std::string, DirectiveValidator>::const_iterator validatorIt = _directiveValidators.find(it->first);
		if (validatorIt != _directiveValidators.end()) {
			(this->*(validatorIt->second))(it->second);
		}
	}

	/* temp, debug */
	std::cout << BLUE "working properly for global directives" << RESET << std::endl;
}


/* j'ai mis le check de location a l'interieur de contextNameCheck pour tester, il faudrat le retirer car la directive location DOIT etre a l'interieur d'un server, on peut pas voir de context bloc location au meme niveau que les servers */
void	Validator::validateServerContexts(void) {

	std::vector<Context>&			contexts = _config.getTokenizer().getVectorContext();
	std::vector<Context>::iterator	it;

	for (it = contexts.begin(); it != contexts.end(); ++it) {
		contextNameCheck(*it);

		_currentContext = &(*it);
		validateContextDirectives(*it, SERV_VALUE);
		_currentContext = NULL;

		it->printBinding();
	}

	/* temp, debug */
	std::cout << GREEN "working properly for server/location directives" << RESET << std::endl;

}

void	Validator::validateContextDirectives(Context& context, int contextType) {

	const std::vector<std::pair<std::string, std::vector<std::string> > >&	directives = context.getDirectives();

	keyNameCheck(directives, contextType);

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		if (it->first == "}")
			continue ;

		if (it->first == LISTEN) {
			validateListen(it->second);
		} else if (it->first == SERV_NAME) {
			validateServerName(it->second);
		} else {
			std::map<std::string, DirectiveValidator>::const_iterator validatorIt = _directiveValidators.find(it->first);
			if (validatorIt != _directiveValidators.end()) {
				(this->*(validatorIt->second))(it->second);
			}
			// TODO: ajouter les validateurs pour les autres directives
		}
	}
}

void	Validator::contextNameCheck(const Context& context) const {

	const std::string&			name = context.getName();
	std::vector<std::string>	group = createVectorFromString(name);

	std::istringstream	ss(name);
	std::string			value;

	ss >> value;
	// std::cout << value << std::endl;
	if (value == SERV) {
		validateStrictArgsNb(group, 2, SERV);
		validateServer(group, context);
	} else if (value == LOCATION) { // this will be moved somewhere else OR validateLocaiton has to change to identify if we are in a location or not, because RN we accept a
		validateStrictArgsNb(group, 3, LOCATION);
		validateLocation(group, context);
	} else {
		std::string errorMsg = "unknown directive \"" + value + "\"";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}
}

// Working for now but I'm not sure I'm done with this function, need to test in depth
void	Validator::validateLocation(const std::vector<std::string>& group, const Context& context) const {

	// std::cout << GREEN << context.getName() << RESET << std::endl;

	if (group.size() != 3) {
		std::string errorMsg = "invalid number of arguments in \"location\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	const std::string& middlePart = group[1];

	if (middlePart.find(";") != std::string::npos) {
		std::string errorMsg = "directive \"location\" has no opening \"{\"";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	} else if (middlePart.find("{") != std::string::npos) {
		std::string errorMsg = "invalid number of arguments in \"location\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	const std::string& bracketPart = group[2];

	if (bracketPart.empty() || bracketPart[0] != '{') {
		std::string errorMsg = "invalid number of arguments in \"location\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	if (bracketPart.length() > 1) {
		char afterBracket = bracketPart[1];
		if (afterBracket == ';' || afterBracket == '{' || afterBracket == '}') {
			std::string	errorMsg = "unexpected \"";
			errorMsg += afterBracket; // WTFFFFF
			errorMsg += "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		} else {
			std::string	unknownPart = bracketPart.substr(1);
			std::string	errorMsg = "unknown directive \"" + unknownPart + "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}
	checkContextClosedProperly(context);
}

void	Validator::validateServer(const std::vector<std::string>& group, const Context& context) const {

	if (group.size() != 2) {
		std::string errorMsg = "invalid number of arguments in \"server\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	const std::string& bracketPart = group[1];

	if (bracketPart.empty() || bracketPart[0] != '{') {
		std::string errorMsg = "invalid number of arguments in \"server\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	if (bracketPart.length() > 1) {
		char afterBracket = bracketPart[1];
		if (afterBracket == ';' || afterBracket == '{' || afterBracket == '}') {
			std::string	errorMsg = "unexpected \"";
			errorMsg += afterBracket; // WTFFFFF
			errorMsg += "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		} else {
			std::string	unknownPart = bracketPart.substr(1);
			std::string	errorMsg = "unknown directive \"" + unknownPart + "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}
	checkContextClosedProperly(context);
}

void	Validator::checkContextClosedProperly(const Context& context) const {

	const std::vector<std::pair<std::string, std::vector<std::string> > >& directives = context.getDirectives();
	const std::string&	lastKey = directives.back().first;

	if (directives.back().second.size() != 0 || lastKey != "}" ) {
		std::string errorMsg = "unexpected end of file, expecting \";\" or \"}\"";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}
}


void	Validator::keyNameCheck(const std::vector<std::pair<std::string, std::vector<std::string> > >& directives, int contextType) const {

	static const char	*allDirectives[] = {
		ERR_PAGE, CL_MAX_B_SYZE, SERV, SERV_NAME, LISTEN, ROOT, INDEX,
		LOCATION, ALL_METHODS, AUTOINDEX, UPLOAD_TO, RETURN, ALIAS, CGI_PATH, CGI_EXT
	};

	const size_t	directivesCount = sizeof(allDirectives) / sizeof(allDirectives[0]);

	const std::vector<std::string>&	allowedDirectives = _allowedInContext[contextType].second;

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		const std::string&	key = it->first;
		bool				found = false;

		/* temp, j'ai peur que ca casse tout du coup mais je reflechirai a ca plus tard*/
		if (key == "}")
			continue ;

		std::vector<std::string>::const_iterator	allowedIt;
		for (allowedIt = allowedDirectives.begin(); allowedIt != allowedDirectives.end(); ++allowedIt) {
			if (key == *allowedIt) {
				found = true;
				break ;
			}
		}

		if (!found) {
			for (size_t i = 0; i < directivesCount; ++i) {
				if (key == allDirectives[i]) {
					std::string errorMsg = "\"" + key + "\" directive is not allowed here";
					Utils::logger(errorMsg, _config.getFilePath());
					throw std::invalid_argument(errorMsg);
				}
			}
			std::string errorMsg = "unknown directive \"" + key + "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}
}

void	Validator::semicolonCheck(const std::vector<std::string>& v, const std::string& directive) const {

	std::vector<std::vector<std::string> >					groups = splitDirectiveGroups(v);
	std::vector<std::vector<std::string> >::const_iterator	groupIt;
	std::string						errorMsg;

	if (groups.empty()) {
		errorMsg = "directive \"" + directive + "\" is not terminated by \";\"";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;
		const std::string&				lastValue = group.back();

		if (lastValue[lastValue.length() - 1] != ';') {
			errorMsg = "directive \"" + directive + "\" is not terminated by \";\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		std::size_t firstSemicolon = lastValue.find(';');

		if (firstSemicolon != lastValue.length() - 1) {

			std::size_t	nextNonSemicolon = lastValue.find_first_not_of(";", firstSemicolon);

			if (nextNonSemicolon != std::string::npos) {
				std::string	unknownPart = lastValue.substr(firstSemicolon + 1);
				unknownPart = unknownPart.substr(0, unknownPart.find_first_of(";"));
				errorMsg = "unknown directive \"" + unknownPart + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			} else {
				errorMsg = "unexpected \";\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}
		}
	}
}


void	Validator::printMap() const {

	std::cout << GREEN "Validator:" RESET << std::endl;

	std::vector<std::pair<std::string, std::vector<std::string> > >::const_iterator it;

	for (it = _config.getTokenizer().getGlobalDirective().begin(); it != _config.getTokenizer().getGlobalDirective().end(); ++it) {
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
			Utils::logger(errorMsg, _config.getFilePath());
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
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (number <= 0) {
			std::string	errorMsg = "\"client_max_body_size\" directive invalid value";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		std::string	unit;
		iss >> unit;

		if (!validateUnity(unit)) {
			std::string errorMsg = "\"client_max_body_size\" directive invalid value";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}

	if (groups.size() > 1) {
		std::string errorMsg = "\"client_max_body_size\" directive is duplicate";
		Utils::logger(errorMsg, _config.getFilePath());
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
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}

			if (iss.peek() != EOF) {
				std::string	errorMsg = "invalid value \"" + group[i] + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}

			if (!(value >= 300 && value <= 599)) {
				std::ostringstream	oss;
				oss << value;
				std::string	errorMsg = "value \"" + oss.str() + "\" must be between 300 and 599";
				Utils::logger(errorMsg, _config.getFilePath());
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
				Utils::logger(errorMsg, _config.getFilePath());
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
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}
}

void	Validator::validateStrictArgsNb(const std::vector<std::string>& group, size_t exactNb, const std::string& directive) const {

	size_t argCount = 0;

	std::vector<std::string>::const_iterator	it;
	for (it = group.begin(); it != group.end(); ++it) {
		if (!it->empty())
			++argCount;
	}

	if (argCount != exactNb) {
		std::string	errorMsg = "invalid number of arguments in \"" + directive + "\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}
}

/* DEBUG FUNCTIONS */
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

void	Validator::printVector(const std::vector<std::string>& v) const {
	std::vector<std::string>::const_iterator	it;

	std::cout << "PRINT_VECTOR: " << std::endl;

	for (it = v.begin(); it != v.end(); ++it) {
		if (*it == " ") {
			std::cout << std::endl;
			continue ;
		}
		std::cout << "[" << *it << "] ";
	}
	std::cout << std::endl;
}
/* END OF DEBUG FUNCTIONS */

/* utilitary functions that will move to the Utils namespace later*/
std::string	Validator::extractContextType(const std::string& contextName) const {

	std::istringstream	iss(contextName);
	std::string			type;

	iss >> type;

	return (type);
}

std::vector<std::string>	Validator::createVectorFromString(const std::string& str) const {

	std::vector<std::string>	res;
	std::istringstream			ss(str);
	std::string					value;

	while (ss >> value) {
		res.push_back(value);
	}

	return (res);
}
/* end of utilitary functions */



/* start of utilitary functions for listen */
bool	Validator::isValidPort(std::string& portStr, int& outPort) const {

	if (portStr.empty())
		return (false);

	std::istringstream	iss(portStr);

	if (!(iss >> outPort))
		return (false);

	if (iss.peek() != EOF)
		return (false);

	if (outPort < 1 || outPort > 65535) {
		std::string errorMsg = "invalid port in \"" + portStr + "\" of the \"listen\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	return (true);
}

bool	Validator::isValidAddress(std::string& address) const {

	if (address == "*") {
		address = "0.0.0.0";
		return (true);
	}
	if (address == "localhost") {
		address = "127.0.0.1";
		return (true);
	}

	std::istringstream	iss(address);
	std::string			octet;
	int					octetCount = 0;

	while (std::getline(iss, octet, '.')) {
		octetCount++;

		if (octet.empty())
			return (false);

		if (octet.length() > 1 && octet[0] == '0')
			return (false);

		std::istringstream	octetStream(octet);
		int					value;

		if (!(octetStream >> value))
			return (false);

		if (octetStream.peek() != EOF)
			return (false);

		if (value < 0 || value > 255) {
			std::string errorMsg = "host not found in \"" + address + "\" of the \"listen\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}

	return (octetCount == 4);
}

void	Validator::subdivideListen(const std::string& listenValue) const {

	std::string	cleanValue = listenValue;

	while (!cleanValue.empty() && cleanValue[cleanValue.length() - 1] == ';')
		cleanValue = cleanValue.substr(0, cleanValue.length() - 1);

	if (cleanValue.empty()) {
		std::string errorMsg = "invalid number of arguments in \"listen\" directive";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	std::size_t	colonPos = cleanValue.find(':');
	std::string	address;
	int			port;

	// address:port
	if (colonPos != std::string::npos) {
		address = cleanValue.substr(0, colonPos);
		std::string	portStr = cleanValue.substr(colonPos + 1);

		if (address.empty() || portStr.empty()) {
			std::string errorMsg = "invalid parameter \"" + cleanValue + "\" in \"listen\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (!isValidAddress(address)) {
			std::string errorMsg = "host not found in \"" + address + "\" of the \"listen\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (!isValidPort(portStr, port)) {
			std::string errorMsg = "invalid port in \"" + cleanValue + "\" of the \"listen\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	} else {
		if (isValidPort(cleanValue, port)) {
			address = "0.0.0.0";
		} else if (isValidAddress(cleanValue)) {
			address = cleanValue;
			port = 80;
		} else {
			std::string errorMsg = "invalid parameter \"" + cleanValue + "\" in \"listen\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}

	// debug
	// std::cout << "Parsed listen: " << address << ":" << port << std::endl;
	_currentContext->addListenPair(address, port, _config.getFilePath());
}
/* end of utilitary functions for listen */

void	Validator::validateListen(const std::vector<std::string>& values) {

	std::cout << RED "entered validateListen" RESET << std::endl;
	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values);

	std::vector<std::vector<std::string> >::const_iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateStrictArgsNb(group, 1, LISTEN);

		subdivideListen(group[0]);
	}

	semicolonCheck(values, LISTEN);
	// printVector(values);
}

bool	Validator::isOnlySemicolons(const std::string& str) const {
	if (str.empty())
		return (false);
	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] != ';')
			return (false);
	}
	return (true);
}

void	Validator::validateServerName(const std::vector<std::string>& values) {

	std::vector<std::vector<std::string> >					groups = splitDirectiveGroups(values);
	std::vector<std::vector<std::string> >::const_iterator	groupIt;

	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>& group = *groupIt;

		validateMinimumArgs(group, 1, SERV_NAME);

		for (size_t i = 0; i + 1 < group.size(); ++i) {

			const std::string& token = group[i];

			if (!token.empty() && token[token.length() - 1] == ';') {

				std::string nextToken = group[i + 1];

				while (!nextToken.empty() && nextToken[nextToken.length() - 1] == ';')
					nextToken = nextToken.substr(0, nextToken.length() - 1);

				std::string	errorMsg;

				if (nextToken.empty()) {
					errorMsg = "unexpected \";\"";
				} else {
					errorMsg = "unknown directive \"" + nextToken + "\"";
				}
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}
		}

		for (size_t i = 0; i < group.size(); ++i) {
			std::string name = group[i];

			while (!name.empty() && name[name.length() - 1] == ';')
				name = name.substr(0, name.length() - 1);

			if (name.empty())
				continue ;

			if (name.find_first_of("*") != std::string::npos) {
				std::string errorMsg = "wildcards rejected in \"" + name + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}
			_currentContext->addServerName(name, _config.getFilePath());
		}
	}
	semicolonCheck(values, SERV_NAME);
}
