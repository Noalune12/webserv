#ifndef SERVERMANAGER_HPP
# define SERVERMANAGER_HPP

#include "ConfigInheritor.hpp"

struct ListenEndPoint
{
	std::string addr;
	int			port;
	int			socketFd;

	// List of servers that are linked/listening to that endpoint (Virtual Hosting)
	std::vector<server*>	servers;

	ListenEndPoint() : addr(), port(0), socketFd(-1), servers() {}
};


class ServerManager
{
	private:
		std::vector<server>			_servers;
		globalDir 					_globalDir;
		std::vector<ListenEndPoint>	_endpoints;

		std::map<int, size_t>		_socketToEndpoint;

		void	closeSockets(void);

	public:
		ServerManager(std::vector<server>& servers, globalDir globalDir);
		~ServerManager();

		// reminder: move to private what can be
		void	setupListenSockets(void);
		void	groupServersByEndPoint(void);

		int		createListenSocket(const std::string& address, int port);
		bool	configureSocket(int socketFd);
		bool	isListenSocket(int fd) const;

		/* debug */
		void	printEndpoints(void);

		std::vector<int>	getListenSocketFds(void);
		std::vector<server>	getServers(void);
		globalDir 			getGlobalDir(void);
};

#endif
