#include <iostream>

#include "FileReader.hpp"
#include "Server.hpp"


Server::Server(const std::string& configFile) : _config(configFile) {

	try {

		/* calls the Facade design pattern for the configuration file here ? */

	} catch(const std::exception& e) {
        std::cerr << "Server initialization failed: " << e.what() << std::endl;
		throw;
	}
}

Server::~Server() {}
