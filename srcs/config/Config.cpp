#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "Config.hpp"
#include "FileReader.hpp"

Config::Config(const std::string& configFile /* nom a revoir j'ai mis autre chose dans mes fichiers de test */) {

	try {

		FileReader reader(configFile);

		_filePath = reader.getFilePath();
		_fileContent = reader.getFileContent();

		Tokenizer temp(_fileContent);
		_tokens = temp;
    	_tokens.printContent();

	} catch(const std::exception& e) {
		std::cerr << "Server initialization failed: " << e.what() << std::endl;
		throw;
	}
}

Config::~Config() {}


