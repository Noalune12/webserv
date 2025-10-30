#include <iostream>

#include "Config.hpp"
#include "FileReader.hpp"

// Config::Config() {}

Config::Config(const std::string& configFile /* nom a revoir j'ai mis autre chose dans mes fichiers de test */) {

	try {

		FileReader reader(configFile);

		_fileContent = reader.getFileContent();

	} catch(const std::exception& e) {
		std::cerr << "Server initialization failed: " << e.what() << std::endl;
	}

}

Config::~Config() {}
