#include <iostream>
#include <algorithm>

#include "colors.h"
#include "FileReader.hpp"
#include "Validator.hpp"

// Config::Config() {}

Config::Config(const std::string& configFile /* nom a revoir j'ai mis autre chose dans mes fichiers de test */) : _filePath(configFile), _fileContent() {

	try {

		FileReader reader(configFile);

		_fileContent = reader.getFileContent();

		// std::cout << _filePath << std::endl;
		// std::cout << _fileContent << std::endl;

		setUpTest();

		Validator	validator(*this);
		validator.validate();

	} catch(const std::exception& e) {
		std::cerr << "Server initialization failed: " << e.what() << std::endl;
		throw;
	}
}

Config::~Config() {}

Config&	Config::getConfig(void) {
	return (*this);
}

void	Config::printMap() const {
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


/* GETTERS */

const std::string&	Config::getFilePath(void) const {
	return (this->_filePath);
}

std::string			Config::getFileContent(void) const {
	return (this->_fileContent);
}

std::map<std::string, std::vector<std::string> >&	Config::getGlobalDirective(void) {
	return (this->_globalDirectives);
}

std::vector<Context>	Config::getVectorContext(void) const {
	return (this->_context);
}


/* TEMP */

void Config::setUpTest(void) {

    /* client_max_body_size */
    std::vector<std::string> cl_max_b_size;

    cl_max_b_size.push_back("10m;");


    /* error_page */
    std::vector<std::string> error_page;

	error_page.push_back("404");
	error_page.push_back("./test-path-/404-test.html;");

    /* error_log */
    std::vector<std::string> error_log;

	error_log.push_back("./var/logs/error.log;");


	/* globalDirectives temporary tester */

    _globalDirectives["client_max_body_size"] = cl_max_b_size;
    _globalDirectives["error_page"] = error_page;
    _globalDirectives["error_log"] = error_log;
}
