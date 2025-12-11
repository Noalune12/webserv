#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "colors.h"
#include "FileReader.hpp"
#include "Validator.hpp"

// Config::Config() {}

Config::Config(const std::string& configFile /* nom a revoir j'ai mis autre chose dans mes fichiers de test */) : _filePath(configFile), _fileContent() {

	try {

		FileReader reader(_filePath);

		_fileContent = reader.getFileContent();

		Tokenizer temp(_fileContent);
		_tokens = temp;
		
		Validator validator(*this);
		validator.validate();
		
    	// _tokens.printContent();

		ConfigInheritor _temp(_tokens);
		
	} catch(const std::exception& e) {
		std::cerr << "Server initialization failed: " << e.what() << std::endl;
		throw;
	}
}

Config::~Config() {}

/* getters */

Tokenizer&	Config::getTokenizer(void) {
	return (this->_tokens);
}

const std::string&	Config::getFilePath(void) const {
	return (this->_filePath);
}

const std::string&	Config::getFileContent(void) const {
	return (this->_fileContent);
}
