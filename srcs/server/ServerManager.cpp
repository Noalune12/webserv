#include <iostream>
#include <sstream>
#include <unistd.h>

#include "colors.h"

#include "ServerManager.hpp"

ServerManager::ServerManager(std::vector<server>& servers) : _servers(servers), _endpoints() /*, _socketToEndpoint()*/ {}

ServerManager::~ServerManager() {
	closeSockets(); // exit properly, might want to put that somewhere else tho
}

void	ServerManager::closeSockets(void) {
	for (size_t i = 0; i < _endpoints.size(); ++i) {
		if (_endpoints[i].socketFd >= 0) {
			close(_endpoints[i].socketFd);
			_endpoints[i].socketFd = -1;
		}
	}
	// _socketToEndpoint.clear();
}


void	ServerManager::setupListenSockets(void) {

	// need to group all servers by listen ip:port -> needed for virtual hosting
	groupServersByEndPoint();

	// debug
	printListenEndPointContent();
}

void	ServerManager::groupServersByEndPoint(void) {

	// uncomment if server reload is necessary
	// _endpoints.clear();

	// Key: ["address:port", index] in _endpoints
	std::map<std::string, size_t> endpointMap;

	for (size_t i = 0; i < _servers.size(); ++i) {

		server& srv = _servers[i];

		// Check and handle multiple listen directives in a single server block
		for (size_t j = 0; j < srv.lis.size(); ++j) {

			const listen& lis = srv.lis[j];

			// Formatting ip:port into key (string) -> 0.0.0.0:8080 for exemple
			std::ostringstream keyStream;
			keyStream << lis.ip << ":" << lis.port;
			std::string key = keyStream.str();

			// Check if we already have this endpoint
			std::map<std::string, size_t>::iterator it = endpointMap.find(key);
			if (it == endpointMap.end()) {
				ListenEndPoint ep;
				ep.addr = lis.ip;
				ep.port = lis.port;
				ep.socketFd = -1;
				ep.servers.push_back(&srv);

				endpointMap[key] = _endpoints.size();
				_endpoints.push_back(ep);
			} else {
				// Existing endpoint, meaning virtual hosting -> add server to current endpoint
				_endpoints[it->second].servers.push_back(&srv);
			}
		}
	}
}


/* DEBUG */
void	ServerManager::printListenEndPointContent(void) {

	std::vector<ListenEndPoint>::const_iterator	it;

	for (it = _endpoints.begin(); it != _endpoints.end(); ++it) {
		std::cout << RED "IP addresss: " << it->addr
			<< "\nPort: " << it->port
			<< "\nSocket fd[" << it->socketFd
			<< "]\nnumber of server_name: " << it->servers.size() << RESET << std::endl;
	}
}
