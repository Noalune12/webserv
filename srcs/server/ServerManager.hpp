#ifndef SERVERMANAGER_HPP
# define SERVERMANAGER_HPP

#include "ConfigInheritor.hpp"

struct ListenEndPoint
{
	std::string _addr;
	int			_port;
	int			_socketFd;

	std::vector<const server*>	_server;

	ListenEndPoint() : _addr(), _port(0), _socketFd(-1), _server() {} // RAII
};


class ServerManager
{
	private:
		std::vector<server>			_servers;
		std::vector<ListenEndPoint>	_endpoints;


		void	closeSockets(void);

	public:
		ServerManager(std::vector<server>& servers);
		~ServerManager();

};

#endif
