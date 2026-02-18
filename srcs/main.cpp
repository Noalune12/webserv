#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "Config.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

#define DEFAULT_CONFIGURATION_FILE "config-files/default.conf"

static EventLoop*	g_eventLoop = NULL;

static bool			g_running = true;

void	signalHandler(int signum) {

	g_running = false;
	if (g_eventLoop) {
		g_eventLoop->stop(); // nothing else, our destructors will manage the ressources release
	}

	write(STDOUT_FILENO, "\033[2K\r", 5);

	std::ostringstream oss;
	oss << "signal " << signum << " received, exiting";
	Logger::notice(oss.str());
	Logger::notice("shutting down...");
}

int	main(int ac, char **av) {

	std::signal(SIGINT, signalHandler);
	std::signal(SIGPIPE, SIG_IGN);

	const static std::string	configFile = (ac > 1) ? av[1] : DEFAULT_CONFIGURATION_FILE;

	Logger::setColor(false); // remove for color

	if (ac > 2) {
		Logger::error("too many arguments");
		return (EXIT_FAILURE);
	}

	try
	{
		Logger::notice("loading configuration file from " + configFile);
		Config	config(configFile);

		if (g_running == true) {
			Logger::notice("configuration loaded successfully");
			Logger::notice("Server startup");
		}

		ServerManager	serverManager(config.getServers(), config.getGlobalDir()); // will setup the informations needed for each servers in their own subclasses
		if (g_running == true)
			serverManager.setupListenSockets();


		EventLoop	eventLoop(serverManager);
		if (g_running == true && !eventLoop.init()) {
			return (EXIT_FAILURE);
		}

		g_eventLoop = &eventLoop;
		if (g_running == true)
			eventLoop.run();
	}
	catch(const std::exception& e)
	{
		Logger::error(std::string("server error: ") + e.what());
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
