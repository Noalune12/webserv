#include <cstdlib>
#include <iostream>
#include <csignal>
#include <sstream>

#include "colors.hpp"
#include "Config.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"

#define DEFAULT_CONFIGURATION_FILE "config-files/default.conf"

static EventLoop*	g_eventLoop = NULL;

void	signalHandler(int signum) {

	if (g_eventLoop) {
		g_eventLoop->stop(); // nothing else, our destructors will manage the ressources release
	}

	write(STDOUT_FILENO, "\033[2K\r", 5);

	std::ostringstream oss;
	oss << "signal " << signum << " received, exiting";
	Logger::notice(oss.str());
	Logger::notice("shutting down...");
}

// add SIGPIPE ? SIGTERM (i don't know if CGIs can cause them) ?
void	setupSignalHandlers(void) {
	signal(SIGINT, signalHandler);
}

int	main(int ac, char **av) {

	const static std::string	configFile = (ac > 1) ? av[1] : DEFAULT_CONFIGURATION_FILE; // not sure this would work in every case, leaving comments below as backup

	try
	{
		Logger::notice("loading configuration file from " + configFile);
		Config	config(configFile);
		Logger::notice("configuration loaded successfully");
		Logger::notice("Server startup");

		ServerManager	serverManager(config.getServers(), config.getGlobalDir()); // -> will setup the informations needed for each servers in their own subclasses
		serverManager.setupListenSockets();

		EventLoop	eventLoop(serverManager);
		eventLoop.init(); // not checking the return value yet

		// ctrl+c only for now
		g_eventLoop = &eventLoop;
		setupSignalHandlers();

		eventLoop.run();

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
		Logger::error(std::string("server initialization failed: ") + e.what());
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
