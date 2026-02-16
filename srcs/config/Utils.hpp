#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>

#include "Context.hpp"

namespace Utils {

	bool	isOnlyWSpace(const std::string& line);

	std::string	handleWSpaceComments(std::string& line);

	Context	handleContext(std::istringstream& f, std::string& content);

	void	logger(const std::string& error, const std::string& filePath);

	void	printDirectives(const PairVector& directives);

	void	unexpectedBracket(PairVector::const_iterator it, const std::string& filePath);
	void	directiveNotTerminatedBySemicolon(PairVector::const_iterator it, const std::string& filePath);
	void	duplicateDirective(std::vector<std::vector<std::string> > groups, const char* directive, const std::string& filePath);

	void	invalidNumberOfArguments(std::vector<std::string>::const_iterator it, const char* directive, const std::string& filePath);
	void	invalidNumberOfArgumentsInContext(const std::string&, const char* context, const std::string& filePath);
}

#endif
