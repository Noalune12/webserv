#ifndef SERVERMANAGER_HPP
# define SERVERMANAGER_HPP

#include "ConfigInheritor.hpp"

struct ListenEndPoint
{
	std::string addr;
	int			port;
	int			socketFd;

	// List of servers that are linked/listening to that endpoint (Virtual Hosting)
	std::vector<const server*>	servers;

	ListenEndPoint() : addr(), port(0), socketFd(-1), servers() {} // RAII
};


class ServerManager
{
	private:
		std::vector<server>			_servers;
		std::vector<ListenEndPoint>	_endpoints;

		// might want to use that for quick search of socketFd to endpoints indexes
		// std::map<int, size_t>		_socketToEndpoint;

		void	closeSockets(void);

	public:
		ServerManager(std::vector<server>& servers);
		~ServerManager();

		// reminder: move to private what can be
		void	setupListenSockets(void);
		void	groupServersByEndPoint(void);
		void	printListenEndPointContent(void);
};

#endif
