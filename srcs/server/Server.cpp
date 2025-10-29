#include <iostream>

// temp
#include "Config.hpp"

#include "FileReader.hpp"
#include "Server.hpp"


Server::Server() : _config() {}

Server::Server(const std::string& configFile) {

	try {
		/* calls the Facade design pattern for the configuration file here ? */
		// this->_config = this->_config.JE_SAIS_PAS_ENCORE();



		// autre design en fonction de ce que j'ai fais pour FileReader.
		FileReader reader(configFile);

		std::string rawConfig = reader.getFileContent();

		// A la place de la ligne au dessus on pourrait avoir
		Config config(reader.getFileContent());

	} catch(const std::exception& e) {
        std::cerr << "Server initialization failed: " << e.what() << std::endl;
	}
}

Server::~Server() {}
