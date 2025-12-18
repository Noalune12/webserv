#include "Utils.hpp"

#include <sstream>
#include <iostream>
#include <fstream>

# define LOG_FILE "var/log/error.log"
# define WEBSERV_PREFIX "webserv: "
# define EMERG "[emerg] "
# define UNKNOWN_DIR "unknown directive "
# define UNEXPECTED "unexpected "
# define CONF_FILE "configuration file "
# define TEST_FAILED "test failed\n"


bool Utils::isOnlyWSpace(const std::string& line) {
	size_t count = 0;

	for (size_t i = 0; i < line.length(); ++i) {
        if (isspace(line[i]))
            count++;
    }
	if (line.length() == count)
		return true;
	return false;
}

void Utils::printDirectives(const std::vector<std::pair<std::string,
               std::vector<std::string> > >& directives) {
	PairVector::const_iterator it;
	for (it = directives.begin(); it != directives.end(); ++it) {
		std::cout << it->first << ": ";

		std::vector<std::string>::const_iterator itv;
		for (itv = it->second.begin(); itv != it->second.end(); ++itv) {
			std::cout << *itv << ", ";
		}
		std::cout << std::endl;
    }
}

std::string Utils::handleWSpaceComments(std::string& line) {
    if (line.empty() || Utils::isOnlyWSpace(line))
		return (line);

    std::istringstream iss(line);
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
    std::size_t	index = line.find('#');
    if (index != std::string::npos)
            line = line.substr(0, index);
    return (line);
}

Context Utils::handleContext(std::istringstream& f, std::string& content) {
    int open;
    std::string line;
    std::string contextContent;
    std::size_t index;
    open = 1;
    while (getline(f, line)) {
        if (line.find('{') != std::string::npos)
            open++;
        else if ((index = line.find('}')) != std::string::npos)
            open--;
        if (open == 0) {
            contextContent.append(line.substr(0, index+1));
            contextContent.push_back('\n');
            break;
        }
        contextContent.append(line);
        contextContent.push_back('\n');
    }
    Context C(content, contextContent);
    if (open == 0) {
        // std::cout << "Content at the end of context handler : " << content << std::endl;
        content = line.substr(index + 1);
        // std::cout << "Content after context handler : " << content << std::endl;

    }
    return (C);
}

void	Utils::logger(const std::string& error, const std::string& filePath) {

	static const char	*outputFile = LOG_FILE;
	std::ofstream		file;


	// Wondering if there is a real need of protecting that... There will always be an error thrown and a message on the cerr anyway
	file.open(outputFile, std::ios::out | std::ios::app);

	file << WEBSERV_PREFIX << EMERG << error << " in " << filePath << std::endl; // will NOT add line of the misconfiguration
	file << WEBSERV_PREFIX << "configuration file " << filePath << " test failed" << std::endl;

	file.close();
}


void    Utils::unexpectedBracket(PairVector::const_iterator it, const std::string& filePath) {
 	if (it->first[0] == ';') {
		std::string errorMsg = "unexpected \";\"";
		Utils::logger(errorMsg, filePath);
		throw std::invalid_argument(errorMsg);
	}
}


void    Utils::directiveNotTerminatedBySemicolon(PairVector::const_iterator it, const std::string& filePath) {
    if ((it->second.empty() && it->first != "}")) {
        std::string errorMsg = "directive \"" + it->first + "\" is not terminated by \";\"";
        Utils::logger(errorMsg, filePath);
        throw std::invalid_argument(errorMsg);
    }
}


void    Utils::invalidNumberOfArguments(std::vector<std::string>::const_iterator it, const char* directive, const std::string& filePath) {
	if (*it == ";") {
		std::string errorMsg = "invalid number of arguments in \"";
        errorMsg += directive;
        errorMsg += "\" directive";
        Utils::logger(errorMsg, filePath);
		throw std::invalid_argument(errorMsg);
	}
}

void    Utils::duplicateDirective(std::vector<std::vector<std::string> > groups, const char* directive, const std::string& filePath) {
	if (groups.size() > 1) {
		std::string errorMsg = "\"";
        errorMsg += directive;
        errorMsg += "\" directive is duplicate";
        Utils::logger(errorMsg, filePath);
		throw std::invalid_argument(errorMsg);
	}
}

void    Utils::invalidNumberOfArgumentsInContext(const std::string& bracketPart, const char* context, const std::string& filePath) {
	if (bracketPart.empty() || bracketPart[0] != '{') {
		std::string errorMsg = "invalid number of arguments in \"";
        errorMsg += context;
        errorMsg += "\" directive";
        Utils::logger(errorMsg, filePath);
		throw std::invalid_argument(errorMsg);
	}

	if (bracketPart.length() > 1) {
		char afterBracket = bracketPart[1];
		if (afterBracket == ';' || afterBracket == '{' || afterBracket == '}') {
			std::string	errorMsg = "unexpected \"";
			errorMsg += afterBracket;
			errorMsg += "\"";
			Utils::logger(errorMsg, filePath);
			throw std::invalid_argument(errorMsg);
		} else {
			std::string	unknownPart = bracketPart.substr(1);
			std::string	errorMsg = "unknown directive \"" + unknownPart + "\"";
			Utils::logger(errorMsg, filePath);
			throw std::invalid_argument(errorMsg);
		}
	}
}
