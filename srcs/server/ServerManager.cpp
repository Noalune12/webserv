#include <unistd.h>

#include "ServerManager.hpp"

ServerManager::ServerManager(std::vector<server>& servers) : _servers(servers), _endpoints() {}

ServerManager::~ServerManager() {
	closeSockets(); // exit properly, might want to put that somewhere else tho
}

void	ServerManager::closeSockets(void) {

	std::vector<ListenEndPoint>::iterator	it;

	for (it = _endpoints.begin(); it != _endpoints.end(); ++it) {
		if (it->_socketFd)
			close(it->_socketFd);
	}
}
