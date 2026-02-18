#include "FileReader.hpp"
#include "Validator.hpp"

Config::Config(const std::string& configFile) : _filePath(configFile), _fileContent() {

	try {

		FileReader reader(_filePath);

		_fileContent = reader.getFileContent();

		_tokens.tokenize(_fileContent);

		Validator validator(*this);
		validator.validate();

		_conf.inherit(_tokens);

	} catch(const std::exception& e) {
		throw ;
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

std::vector<server>&	Config::getServers(void) {
	return (this->_conf.getServers());
}

globalDir& Config::getGlobalDir(void) {
	return (this->_conf.getGlobalDir());
}
