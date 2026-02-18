#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

// #include "colors.hpp"
#include "error_messages.h"
#include "rules.h"
#include "Utils.hpp"
#include "Validator.hpp"

#define ALIAS_DUP_ROOT_SPECIFIED  "\"alias\" directive is duplicate, \"root\" directive was specified earlier"
#define ALL_METHOD_REQ "\" in \"allow_methods\" directive, it must be \"GET\", \"POST\" or \"DELETE\""
#define AUTOINDEX_ON_OFF_MUST "\" in \"autoindex\" directive, it must be \"on\" or \"off\""
#define CL_MAX_B_SIZE_INV_VALUE "\"client_max_body_size\" directive invalid value"
#define DIR_NOT_ALLOWED_SUFFIX "\" directive is not allowed here"
#define DUP_LOCATION "duplicate location \""
#define HOST_NOT_FOUND "host not found in \""
#define IN_LISTEN_DIR "\" in \"listen\" directive"
#define LISTEN_SUFFIX "\" of the \"listen\" directive"
#define LOC_NO_OPENING_BRACKET "directive \"location\" has no opening \"{\""
#define NESTED_LOCATION "nested location blocks are not allowed"
#define ROOT_DUP_ALIAS_SPECIFIED "\"root\" directive is duplicate, \"alias\" directive was specified earlier"
#define UNEXPECTED_EOF "unexpected end of file, expecting \";\" or \"}\""
#define UPLOAD_POST_PAIRING_REJECTION "\"upload_to\" directive requires \"allow_methods\" directive with POST argument in the same location"
#define UPLOAD_TO_PATH_REQ "\"upload_to\" directive requires an absolute path, got \""
#define WILDCARD_REJECTION "wildcards rejected in \""

#define INV_CHAR_IDENTIFIED "invalid characters identified in \""
#define INV_METHOD "invalid method \""
#define INV_NB_ARG_LISTEN "invalid number of arguments in \"listen\" directive"
#define INV_NB_ARG_LOCATION "invalid number of arguments in \"location\" directive"
#define INV_NB_ARG_SERVER "invalid number of arguments in \"server\" directive"
#define INV_PARAM "invalid parameter \""
#define INV_PORT "invalid port in \""
#define INV_RET_CODE "invalid return code \""
#define INV_VALUE "invalid value \""

static const int	error_codes[] = {
	301, 302, 303, 307, 308,
	400, 403, 404, 405, 408, 413,
	500, 501, 502, 504, 505
};


Validator::Validator(Config& config) : _config(config), _currentContext(NULL), _allowedInContext() {
	initAllowedContext();
	initValidators();
}

Validator::~Validator() {}

void	Validator::validate(void) {

	validateGlobalDirective();
	validateServerContexts();
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
}

void	Validator::initValidators(void) {

	_directiveValidators[CL_MAX_B_SYZE] = &Validator::validateClientMaxBodySize;
	_directiveValidators[ERR_PAGE] = &Validator::validateErrorPage;
	_directiveValidators[ROOT] = &Validator::validateRoot;
	_directiveValidators[INDEX] = &Validator::validateIndex;
	_directiveValidators[AUTOINDEX] = &Validator::validateAutoIndex;
	_directiveValidators[ALL_METHODS] = &Validator::validateAllowedMethods;
	_directiveValidators[RETURN] = &Validator::validateReturn;
	_directiveValidators[CGI_PATH] = &Validator::validateCGIPath;
	_directiveValidators[CGI_EXT] = &Validator::validateCGIExt;
	_directiveValidators[UPLOAD_TO] = &Validator::validateUploadTo;
	_directiveValidators[ALIAS] = &Validator::validateAlias;
}

void	Validator::rejectSoleBrackets(const PairVector& directives) const {

	PairVector::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it)
	{
		const std::string &name = it->first;
		if (name.empty())
			continue ;
		char c = name[0];
		if (c == '{' || c == '}') {
			std::string errorMsg = std::string(UNEXPECTED) + c + "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}
}

void	Validator::validateGlobalDirective(void) const {

	const PairVector& directives = _config.getTokenizer().getGlobalDirective();
	PairVector::const_iterator	it = directives.begin();

	if (directives.empty())
		return ;

	rejectSoleBrackets(directives);

	for (it = directives.begin(); it != directives.end(); ++it) {

		Utils::unexpectedBracket(it, _config.getFilePath());
		Utils::directiveNotTerminatedBySemicolon(it, _config.getFilePath());
		std::map<std::string, DirectiveValidator>::const_iterator validatorIt = _directiveValidators.find(it->first);
		if (validatorIt != _directiveValidators.end()) {
			(this->*(validatorIt->second))(it->second);
		}
	}
	keyNameCheck(directives, GLOBAL_VALUE);
}

void	Validator::rejectAliasRootInSameLocation(const Context& context) const {

	const	PairVector&	directives = context.getDirectives();

	bool	hasRoot = false;
	bool	hasAlias = false;

	PairVector::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		if (it->first ==  ROOT) {
			if (hasAlias == true) {
				Utils::logger(ROOT_DUP_ALIAS_SPECIFIED, _config.getFilePath());
				throw std::invalid_argument(ROOT_DUP_ALIAS_SPECIFIED);
			}
			hasRoot = true;
		}
		if (it->first == ALIAS) {
			if (hasRoot == true) {
				Utils::logger(ALIAS_DUP_ROOT_SPECIFIED, _config.getFilePath());
				throw std::invalid_argument(ALIAS_DUP_ROOT_SPECIFIED);
			}
			hasAlias = true;
		}
	}
}

void	Validator::rejectDuplicateLocation(const Context& serverContext) const {

	const std::vector<Context>&	locations = serverContext.getContext();
	std::vector<std::string>	seenPaths;

	for (size_t i = 0; i < locations.size(); ++i) {

		std::string			name = locations[i].getName();
		std::istringstream	iss(name);
		std::string			keyword, path;

		iss >> keyword >> path;

		for (size_t j = 0; j < seenPaths.size(); ++j) {
			if (seenPaths[j] == path) {
				std::string errorMsg = DUP_LOCATION + path + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}
		}
		seenPaths.push_back(path);
	}
}

void	Validator::validateLocationContexts(Context& serverContext) {

	std::vector<Context>&			locations = serverContext.getContext();
	std::vector<Context>::iterator	it;

	for (it = locations.begin(); it != locations.end(); ++it) {

		contextNameCheck(*it, LOCATION_VALUE);

		validateContextDirectives(*it, LOCATION_VALUE);

		validateCGIPairing(*it);
		validatePostUploadToPairing(*it);
		rejectAliasRootInSameLocation(*it);

		if (!it->getContext().empty()) {
			Utils::logger(NESTED_LOCATION, _config.getFilePath());
			throw std::invalid_argument(NESTED_LOCATION);
		}
	}
	rejectDuplicateLocation(serverContext);
}

void	Validator::validateServerContexts(void) {

	std::vector<Context>&			contexts = _config.getTokenizer().getVectorContext();
	std::vector<Context>::iterator	it;

	for (it = contexts.begin(); it != contexts.end(); ++it) {

		contextNameCheck(*it, SERV_VALUE);

		_currentContext = &(*it);
		validateContextDirectives(*it, SERV_VALUE);
		validateLocationContexts(*it);
		_currentContext = NULL;
	}

	validateVirtualHostConflicts();
}

void	Validator::validateContextDirectives(Context& context, int contextType) {

	const PairVector&	directives = context.getDirectives();

	keyNameCheck(directives, contextType);

	PairVector::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {

		Utils::directiveNotTerminatedBySemicolon(it, _config.getFilePath());

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
		}
	}
}

void	Validator::validateAutoIndex(const std::vector<std::string>& values) const {
	std::vector<std::string>::const_iterator	it = values.begin();

	Utils::invalidNumberOfArguments(it, AUTOINDEX, _config.getFilePath());

	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, AUTOINDEX);

	validateStrictArgsNb(groups[0], 1, AUTOINDEX);

	std::string	value = groups[0][0];
	while (!value.empty() && value[value.length() - 1] == ';')
		value = value.substr(0, value.length() - 1);

	if (value != "on" && value != "off") {
		std::string errorMsg = INV_VALUE + value + AUTOINDEX_ON_OFF_MUST;
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	semicolonCheck(groups[0], AUTOINDEX);
	Utils::duplicateDirective(groups, AUTOINDEX, _config.getFilePath());
}

void	Validator::validateRoot(const std::vector<std::string>& values) const {
	std::vector<std::string>::const_iterator it = values.begin();
	Utils::invalidNumberOfArguments(it, ROOT, _config.getFilePath());
	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, ROOT);
	validateStrictArgsNb(groups[0], 1, ROOT);
	semicolonCheck(groups[0], ROOT);
	Utils::duplicateDirective(groups, ROOT, _config.getFilePath());
}

void	Validator::validateAlias(const std::vector<std::string>& values) const {
	std::vector<std::string>::const_iterator it = values.begin();
	Utils::invalidNumberOfArguments(it, ALIAS, _config.getFilePath());
	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, ALIAS);
	validateStrictArgsNb(groups[0], 1, ALIAS);
	semicolonCheck(groups[0], ALIAS);
	Utils::duplicateDirective(groups, ALIAS, _config.getFilePath());
}


void	Validator::validateIndex(const std::vector<std::string>& values) const {
	std::vector<std::string>::const_iterator it = values.begin();
	Utils::invalidNumberOfArguments(it, INDEX, _config.getFilePath());
	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, INDEX);
	validateMinimumArgs(groups[0], 1, INDEX);
	semicolonCheck(values, INDEX);
	Utils::duplicateDirective(groups, INDEX, _config.getFilePath());
}

void	Validator::validateUploadTo(const std::vector<std::string>& values) const {
	std::vector<std::string>::const_iterator it = values.begin();
	Utils::invalidNumberOfArguments(it, UPLOAD_TO, _config.getFilePath());
	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, UPLOAD_TO);
	validateStrictArgsNb(groups[0], 1, UPLOAD_TO);
	semicolonCheck(groups[0], UPLOAD_TO);
	const std::vector<std::string>&	group = groups[0];
	std::string	pathValue = group[0];
	if (pathValue[0] != '/') {
		std::string	errorMsg = UPLOAD_TO_PATH_REQ + pathValue + "\"";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}
	Utils::duplicateDirective(groups, UPLOAD_TO, _config.getFilePath());
}

void	Validator::validateAllowedMethods(const std::vector<std::string>& values) const {
	std::vector<std::string>::const_iterator	it = values.begin();
	Utils::invalidNumberOfArguments(it, ALL_METHODS, _config.getFilePath());
	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, ALL_METHODS);
	validateMinimumArgs(groups[0], 1, ALL_METHODS);

	for (size_t i = 0; i < groups[0].size(); ++i) {
		std::string value = groups[0][i];

		while (!value.empty() && value[value.length() - 1] == ';')
			value = value.substr(0, value.length() - 1);

		if (value.empty())
			continue;

		if (value != "GET" && value != "POST" && value != "DELETE") {
			std::string errorMsg = INV_METHOD + value + ALL_METHOD_REQ;
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}

	semicolonCheck(groups[0], ALL_METHODS);
	Utils::duplicateDirective(groups, ALL_METHODS, _config.getFilePath());
}

void	Validator::contextNameCheck(const Context& context, int expectedType) const {

	const std::string&			name = context.getName();
	std::vector<std::string>	group = createVectorFromString(name);

	std::istringstream	ss(name);
	std::string			value;

	ss >> value;
	if (expectedType == SERV_VALUE) {
		if (value != SERV) {
			std::string errorMsg = "\"" + value + DIR_NOT_ALLOWED_SUFFIX;
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
		validateStrictArgsNb(group, 2, SERV);
		validateServer(group, context);
	} else if (expectedType == LOCATION_VALUE) {
		if (value != LOCATION) {
			std::string errorMsg = "\"" + value + DIR_NOT_ALLOWED_SUFFIX;
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
		validateStrictArgsNb(group, 3, LOCATION);
		validateLocation(group, context);
	} else {
		std::string errorMsg = UNKNOWN_DIR_PREFIX + value + "\"";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}
}

// Working for now but I'm not sure I'm done with this function, need to test in depth - update looks good to me
void	Validator::validateLocation(const std::vector<std::string>& group, const Context& context) const {

	if (group.size() != 3) {
		Utils::logger(INV_NB_ARG_LOCATION, _config.getFilePath());
		throw std::invalid_argument(INV_NB_ARG_LOCATION);
	}

	const std::string& middlePart = group[1];

	if (middlePart.find(";") != std::string::npos) {
		Utils::logger(LOC_NO_OPENING_BRACKET, _config.getFilePath());
		throw std::invalid_argument(LOC_NO_OPENING_BRACKET);
	} else if (middlePart.find("{") != std::string::npos) {
		Utils::logger(INV_NB_ARG_LOCATION, _config.getFilePath());
		throw std::invalid_argument(INV_NB_ARG_LOCATION);
	}

	const std::string& bracketPart = group[2];
	Utils::invalidNumberOfArgumentsInContext(bracketPart, LOCATION, _config.getFilePath());
	checkContextClosedProperly(context);
}

void	Validator::validateServer(const std::vector<std::string>& group, const Context& context) const {

	if (group.size() != 2) {
		Utils::logger(INV_NB_ARG_SERVER, _config.getFilePath());
		throw std::invalid_argument(INV_NB_ARG_SERVER);
	}

	const std::string& bracketPart = group[1];
	Utils::invalidNumberOfArgumentsInContext(bracketPart, SERV, _config.getFilePath());
	checkContextClosedProperly(context);
}


// dead function with the actual Utils::handleContext behavior (open boolean behavior, might want to move the error message to that port of the code)
void	Validator::checkContextClosedProperly(const Context& context) const {

	const PairVector& directives = context.getDirectives();
	const std::string&	lastKey = directives.back().first;

	if (directives.back().second.size() != 0 || lastKey != "}" ) {
		Utils::logger(UNEXPECTED_EOF, _config.getFilePath());
		throw std::invalid_argument(UNEXPECTED_EOF);
	}
}


void	Validator::keyNameCheck(const PairVector& directives, int contextType) const {

	static const char	*allDirectives[] = {
		ERR_PAGE, CL_MAX_B_SYZE, SERV, SERV_NAME, LISTEN, ROOT, INDEX,
		LOCATION, ALL_METHODS, AUTOINDEX, UPLOAD_TO, RETURN, ALIAS, CGI_PATH, CGI_EXT
	};

	const size_t	directivesCount = sizeof(allDirectives) / sizeof(allDirectives[0]);

	const std::vector<std::string>&	allowedDirectives = _allowedInContext[contextType].second;

	PairVector::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		std::string	key = it->first;
		bool		found = false;

		// check si cette boucle est toujours necessaire suite au changement que tu as fais le 17/12
		while (!key.empty() && key[key.length() - 1] == ';') {
			key = key.substr(0, key.length() - 1);
		}

		if (key == "}" || (!key.empty() && key[0] == '}'))
			continue ;

		Utils::unexpectedBracket(it, _config.getFilePath());

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
					std::string errorMsg = "\"" + key + DIR_NOT_ALLOWED_SUFFIX;
					Utils::logger(errorMsg, _config.getFilePath());
					throw std::invalid_argument(errorMsg);
				}
			}
			std::string errorMsg = UNKNOWN_DIR_PREFIX + key + "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}
}

void	Validator::semicolonCheck(const std::vector<std::string>& v, const std::string& directive) const {

	std::vector<std::vector<std::string> >					groups = splitDirectiveGroups(v, directive);
	std::vector<std::vector<std::string> >::const_iterator	groupIt;
	std::string						errorMsg;

	if (groups.empty()) {
		errorMsg = DIRECTIVE_PREFIX + directive + NOT_TERMINATED_BY_SEMICOLON;
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;
		const std::string&				lastValue = group.back();

		if (lastValue[lastValue.length() - 1] != ';') {
			errorMsg = DIRECTIVE_PREFIX + directive + NOT_TERMINATED_BY_SEMICOLON;
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		std::size_t firstSemicolon = lastValue.find(';');

		if (firstSemicolon != lastValue.length() - 1) {

			std::size_t	nextNonSemicolon = lastValue.find_first_not_of(";", firstSemicolon);

			if (nextNonSemicolon != std::string::npos) {
				std::string	unknownPart = lastValue.substr(firstSemicolon + 1);
				unknownPart = unknownPart.substr(0, unknownPart.find_first_of(";"));
				errorMsg = UNKNOWN_DIR_PREFIX + unknownPart + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			} else {
				Utils::logger(UNEXP_SEMICOLON, _config.getFilePath());
				throw std::invalid_argument(UNEXP_SEMICOLON);
			}
		}
	}
}

static bool	validateUnity(const std::string& leftover) {
	if (leftover.length() != 1)
		return (false);
	return (leftover[0] == 'k' || leftover[0] == 'K' || leftover[0] == 'm' || leftover[0] == 'M' || leftover[0] == 'g' || leftover[0] == 'G');
}

void	Validator::validateClientMaxBodySize(const std::vector<std::string>& values) const {

	std::vector<std::string>::const_iterator	it = values.begin();
	Utils::invalidNumberOfArguments(it, CL_MAX_B_SYZE, _config.getFilePath());

	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, CL_MAX_B_SYZE);

	std::vector<std::vector<std::string> >::const_iterator	groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateStrictArgsNb(group, 1, CL_MAX_B_SYZE);

		std::string	cleanValue = group[0];
		while (!cleanValue.empty() && cleanValue[cleanValue.length() - 1] == ';') {
			cleanValue = cleanValue.substr(0, cleanValue.length() - 1);
		}

		std::istringstream	iss(cleanValue);
		long				number;

		if (!(iss >> number)) {
			Utils::logger(CL_MAX_B_SIZE_INV_VALUE, _config.getFilePath());
			throw std::invalid_argument(CL_MAX_B_SIZE_INV_VALUE);
		}

		if (number <= 0) {
			Utils::logger(CL_MAX_B_SIZE_INV_VALUE, _config.getFilePath());
			throw std::invalid_argument(CL_MAX_B_SIZE_INV_VALUE);
		}

		std::string	unit;
		iss >> unit;

		if (!validateUnity(unit)) {
			Utils::logger(CL_MAX_B_SIZE_INV_VALUE, _config.getFilePath());
			throw std::invalid_argument(CL_MAX_B_SIZE_INV_VALUE);
		}
	}

	Utils::duplicateDirective(groups, CL_MAX_B_SYZE, _config.getFilePath());
	semicolonCheck(values, CL_MAX_B_SYZE);
}

void	Validator::validateErrorPage(const std::vector<std::string>& values) const {

	std::vector<std::string>::const_iterator	it = values.begin();
	Utils::invalidNumberOfArguments(it, ERR_PAGE, _config.getFilePath());
	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, ERR_PAGE);

	std::vector<std::vector<std::string> >::const_iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateMinimumArgs(group, 2, ERR_PAGE);

		for (size_t i = 0; i < group.size() - 1; ++i) {

			std::istringstream	iss(group[i]);
			int					value;

			if (!(iss >> value)) {
				std::string	errorMsg = INV_VALUE + group[i] + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}

			if (iss.peek() != EOF) {
				std::string	errorMsg = INV_VALUE + group[i] + "\"";
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

			if (!isValidErrorCode(value)) {
				std::ostringstream	oss;
				oss << value;
				std::string errorMsg = INV_VALUE + oss.str() + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}
		}
	}
	semicolonCheck(values, ERR_PAGE);
}

std::vector<std::vector<std::string> >	Validator::splitDirectiveGroups(const std::vector<std::string>& values, const std::string& directive) const {

	std::vector<std::vector<std::string> >	groups;
	std::vector<std::string>				current;

	std::vector<std::string>::const_iterator	it;
	std::string									errorMsg;

	if (values.back() == " ") {
		errorMsg = DIRECTIVE_PREFIX + directive + NOT_TERMINATED_BY_SEMICOLON;
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

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
		std::string	errorMsg = INV_NB_ARG + directive + DIRECTIVE_SUFFIX;
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
		std::string	errorMsg = INV_NB_ARG + directive + DIRECTIVE_SUFFIX;
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

/* utilitary functions that will move to the Utils namespace later, not used anymore */
std::vector<std::string>	Validator::createVectorFromString(const std::string& str) const {

	std::vector<std::string>	res;
	std::istringstream			ss(str);
	std::string					value;

	while (ss >> value) {
		res.push_back(value);
	}

	return (res);
}

bool	Validator::isValidErrorCode(int code) const {

	const size_t	validCodeCount = sizeof(error_codes) / sizeof(error_codes[0]);

	bool	validCode = false;
	for (size_t j = 0; j < validCodeCount; ++j) {
		if (error_codes[j] == code) {
			validCode = true;
			break ;
		}
	}
	return (validCode);
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
		std::string errorMsg = INV_PORT + portStr + LISTEN_SUFFIX;
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

	if (address.empty() || address[0] == '.' || address[address.length() - 1] == '.')
		return (false);

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
			std::string errorMsg = HOST_NOT_FOUND + address + LISTEN_SUFFIX;
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
		Utils::logger(INV_NB_ARG_LISTEN, _config.getFilePath());
		throw std::invalid_argument(INV_NB_ARG_LISTEN);
	}

	std::size_t	colonPos = cleanValue.find(':');
	std::string	address;
	int			port;

	if (colonPos != std::string::npos) {
		address = cleanValue.substr(0, colonPos);
		std::string	portStr = cleanValue.substr(colonPos + 1);

		if (address.empty() || portStr.empty()) {
			std::string errorMsg = INV_PARAM + cleanValue + IN_LISTEN_DIR;
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (!isValidAddress(address)) {
			std::string errorMsg = HOST_NOT_FOUND + address + LISTEN_SUFFIX;
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (!isValidPort(portStr, port)) {
			std::string errorMsg = INV_PORT + cleanValue + LISTEN_SUFFIX;
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
			std::string errorMsg = INV_PARAM + cleanValue + IN_LISTEN_DIR;
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}
	_currentContext->addListenPair(address, port, _config.getFilePath());
}
/* end of utilitary functions for listen */

void	Validator::validateListen(const std::vector<std::string>& values) {

	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, LISTEN);

	std::vector<std::vector<std::string> >::const_iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateStrictArgsNb(group, 1, LISTEN);

		subdivideListen(group[0]);
	}

	semicolonCheck(values, LISTEN);
}


void	Validator::validateServerName(const std::vector<std::string>& values) {

	std::vector<std::vector<std::string> >					groups = splitDirectiveGroups(values, SERV_NAME);
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
					errorMsg = UNEXP_SEMICOLON;
				} else {
					errorMsg = UNKNOWN_DIR_PREFIX + nextToken + "\"";
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
				std::string errorMsg = WILDCARD_REJECTION + name + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}

			std::string	delimiters = "!\"#$%&'()+,/:;<=>?@[\\]^`{|}~\t\n\r";
			if (name.find_first_of(delimiters) != std::string::npos) {
				std::string errorMsg = INV_CHAR_IDENTIFIED + name + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}

			_currentContext->addServerName(name, _config.getFilePath());
		}
	}
	semicolonCheck(values, SERV_NAME);
}

void	Validator::validateVirtualHostConflicts(void) const {

	std::vector<Context>&		servers = _config.getTokenizer().getVectorContext();

	std::map<std::string, size_t>	bindingMap;

	for (size_t i = 0; i < servers.size(); ++i) {

		Bindings&	binding = servers[i].getBinding();

		if (binding.listenPairs.empty()) {
			binding.listenPairs.push_back(std::make_pair("0.0.0.0", 80));
		}

		std::vector<std::string>&	names = binding.serverNames;
		if (names.empty()) {
			names.push_back("");
		}

		for (size_t j = 0; j < binding.listenPairs.size(); ++j) {

			const std::string&	addr = binding.listenPairs[j].first;
			int					port = binding.listenPairs[j].second;

			std::ostringstream	addrPortKey;
			addrPortKey << addr << ":" << port;
			std::string	addrPortStr = addrPortKey.str();

			for (size_t k = 0; k < names.size(); ++k) {

				std::string	key = addrPortStr + ":" + names[k];

				std::map<std::string, size_t>::const_iterator existing = bindingMap.find(key);
				if (existing != bindingMap.end()) {
					std::ostringstream	oss;
					std::cerr << "webserv: [warn] conflicting server name \"" << names[k] << "\" on " << addr << ":" << port << " , ignored " << std::endl;
					servers[i].setIsIgnored();
				}
				bindingMap[key] = i;
			}
		}
	}
}


/* CGI */
void	Validator::validateCGIPath(const std::vector<std::string>& values) const {

	std::vector<std::string>::const_iterator	it = values.begin();
	Utils::invalidNumberOfArguments(it, CGI_PATH, _config.getFilePath());


	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, CGI_PATH);

	std::vector<std::vector<std::string> >::const_iterator	groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateStrictArgsNb(group, 1, CGI_PATH);

		std::string	pathValue = group[0];
		while (!pathValue.empty() && pathValue[pathValue.length() - 1] == ';')
			pathValue = pathValue.substr(0, pathValue.length() - 1);

		if (pathValue.empty()) {
			std::string	errorMsg = "invalid value in \"cgi_path\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (pathValue[0] != '/') {
			std::string	errorMsg = "\"cgi_path\" directive requires an absolute path, got \"" + pathValue + "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}

	Utils::duplicateDirective(groups, CGI_PATH, _config.getFilePath());
	semicolonCheck(values, CGI_PATH);
}

void	Validator::validateCGIExt(const std::vector<std::string>& values) const {

	std::vector<std::string>::const_iterator it = values.begin();
	Utils::invalidNumberOfArguments(it, CGI_EXT, _config.getFilePath());

	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, CGI_EXT);

	std::vector<std::vector<std::string> >::const_iterator	groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateStrictArgsNb(group, 1, CGI_EXT);

		std::string	extValue = group[0];
		while (!extValue.empty() && extValue[extValue.length() - 1] == ';')
			extValue = extValue.substr(0, extValue.length() - 1);

		if (extValue.empty()) {
			std::string	errorMsg = "invalid value in \"cgi_ext\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (extValue[0] != '.') {
			std::string	errorMsg = "\"cgi_ext\" directive requires extension starting with '.', got \"" + extValue + "\"";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}

		if (extValue.length() < 2) {
			std::string	errorMsg = "invalid extension \"" + extValue + "\" in \"cgi_ext\" directive";
			Utils::logger(errorMsg, _config.getFilePath());
			throw std::invalid_argument(errorMsg);
		}
	}

	Utils::duplicateDirective(groups, CGI_EXT, _config.getFilePath());
	semicolonCheck(values, CGI_EXT);
}

void	Validator::validateCGIPairing(const Context& context) const {

	const PairVector&	directives = context.getDirectives();

	bool	hasCGIPath = false;
	bool	hasCGIExt = false;

	PairVector::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		if (it->first == CGI_PATH)
			hasCGIPath = true;
		if (it->first == CGI_EXT)
			hasCGIExt = true;
	}

	if (hasCGIExt && !hasCGIPath) {
		std::string	errorMsg = "\"cgi_ext\" directive requires \"cgi_path\" directive in the same location";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}

	if (hasCGIPath && !hasCGIExt) {
		std::string errorMsg = "\"cgi_path\" directive requires \"cgi_ext\" directive in the same location";
		Utils::logger(errorMsg, _config.getFilePath());
		throw std::invalid_argument(errorMsg);
	}
}


void	Validator::validateReturn(const std::vector<std::string>& values) const {

	std::vector<std::string>::const_iterator	it = values.begin();

	Utils::invalidNumberOfArguments(it, RETURN, _config.getFilePath());

	std::vector<std::vector<std::string> >	groups = splitDirectiveGroups(values, RETURN);
	semicolonCheck(values, RETURN);


	std::vector<std::vector<std::string> >::const_iterator groupIt;
	for (groupIt = groups.begin(); groupIt != groups.end(); ++groupIt) {

		const std::vector<std::string>&	group = *groupIt;

		validateStrictArgsNb(group, 2, RETURN);

		for (size_t i = 0; i < group.size() - 1; ++i) {

			std::istringstream	iss(group[i]);
			int					value;

			if (!(iss >> value)) {
				std::string	errorMsg = INV_RET_CODE + group[i] + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}

			if (iss.peek() != EOF) {
				std::string	errorMsg = INV_RET_CODE + group[i] + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}

			if (!isValidErrorCode(value)) {
				std::ostringstream	oss;
				oss << value;
				std::string errorMsg = INV_VALUE + oss.str() + "\"";
				Utils::logger(errorMsg, _config.getFilePath());
				throw std::invalid_argument(errorMsg);
			}
		}
	}
}

void	Validator::validatePostUploadToPairing(const Context& context) const {

	const PairVector&	directives = context.getDirectives();

	bool	hasPostMethod = false;
	bool	hasUploadto = false;

	PairVector::const_iterator	it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		const std::vector<std::string> arg = it->second;
		if (it->first == ALL_METHODS && (std::find(arg.begin(), arg.end(), "POST") != arg.end() || std::find(arg.begin(), arg.end(), "POST;") != arg.end()))
			hasPostMethod = true;
		if (it->first == UPLOAD_TO)
			hasUploadto = true;
	}

	if (hasUploadto && !hasPostMethod) {
		Utils::logger(UPLOAD_POST_PAIRING_REJECTION, _config.getFilePath());
		throw std::invalid_argument(UPLOAD_POST_PAIRING_REJECTION);
	}
}
