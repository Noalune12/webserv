#include <cstdlib>
#include <iostream>

#include "colors.h"
#include "Config.hpp"
#include "ServerManager.hpp"

#define DEFAULT_CONFIGURATION_FILE "config-files/default.conf"

int	main(int ac, char **av) {

	const static std::string	configFile = (ac > 1) ? av[1] : DEFAULT_CONFIGURATION_FILE; // not sure this would work in every case, leaving comments below as backup
	// std::cout << configFile << std::endl;
	// return (0);

	// if (ac == 2) {
	// 	configFile = av[1];
	// }
	// else {
	// 	configFile = DEFAULT_CONFIGURATION_FILE;
	// }


	try
	{
		std::cout << BLUE "Loading configuration via Facade" RESET << std::endl;
		Config	config(configFile);

		std::cout << GREEN "Server setup" RESET << std::endl;
		std::vector<server>& servers = config.getServers();
		// (void) servers;
		ServerManager	serverManager(servers); // -> will setup the informations needed for each servers in their own subclasses


		/*
			event loop (fil de controle): correcpond a la file d'evements qui peuvent declencher des execution
			Faire en sorte que cette loop gere les events de facon asynchrone

			Ecoute en continue -> execution asynchrone des methodes,
			De maniere a faire en sorte que les requetes soient non bloquantes

			methode qui se termine -> retour a l'event loop (je sais plus pourquoi j'ai écris ca je comprend plus lol)

		*/
	}
	catch(const std::exception& e)
	{
		return (EXIT_FAILURE);
	}

	std::cout << YELLOW "Closing server properly" RESET << std::endl;
	return (EXIT_SUCCESS);
}
