#include "Server.hpp"

#include <iostream>

Server::Server() : _config() {}

Server::Server(const std::string& configFile) {

	try
	{
		/* calls the Facade design pattern for the configuration file here ? */
		// this->_config = this->_config.JE_SAIS_PAS_ENCORE();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}

Server::~Server() {}
