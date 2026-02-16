#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sstream>
#include <unistd.h>
# include <iostream>

#include "colors.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"

static uint32_t	ipv4_str_to_int(const std::string &address);

ServerManager::ServerManager(std::vector<server>& servers, globalDir globalDir) : _servers(servers), _globalDir(globalDir), _endpoints(), _socketToEndpoint() {}

ServerManager::~ServerManager() {
	closeSockets();
}

void	ServerManager::closeSockets(void) {
	for (size_t i = 0; i < _endpoints.size(); ++i) {
		if (_endpoints[i].socketFd >= 0) {
			close(_endpoints[i].socketFd);
			_endpoints[i].socketFd = -1;
		}
	}
	_socketToEndpoint.clear();
}

void	ServerManager::setupListenSockets(void) {

	// grouping all servers by listen ip:port -> needed for virtual hosting
	groupServersByEndPoint();

	// reason for this is if the user wants to bind sockets on ip/port that are not allowed, we have to try them all and them tell then its not possible to open the server
	bool	oneSuccess = false;

	for (size_t i = 0; i < _endpoints.size(); ++i) {

		ListenEndPoint& ep = _endpoints[i];

		int sockFd = createListenSocket(ep.addr, ep.port);
		if (sockFd < 0) {
			std::cerr << RED << "Failed to create socket for " << ep.addr << ":" << ep.port << RESET << std::endl;
			continue ;
		}

		ep.socketFd = sockFd;
		_socketToEndpoint[sockFd] = i;
		oneSuccess = true;

		for (size_t j = 0; j < ep.servers.size(); ++j) {
			ep.servers[j]->isRunning = true;
		}

		std::ostringstream oss;
		oss << "listening on " << ep.addr << ":" << ep.port;
		Logger::notice(oss.str());
	}

	if (!oneSuccess) {
		std::cerr << RED << "Not a single socket has been created." << RESET << std::endl;
		return ;
	}
	// prettier debug
	// printEndpoints(); // call to Logger in the loop above is enough
}

int	ServerManager::createListenSocket(const std::string& address, int port) {

	int	socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0) {
		// throw error instead ?
		std::cerr << "socket() failed: " << strerror(errno) << std::endl;
		return (-1); // return -1 for now for error reporting
	}

	if (!configureSocket(socketFd))
	{
		close(socketFd);
		return (-1);
	}

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = ipv4_str_to_int(address);

	if (bind(socketFd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		std::cerr << "bind() failed: " << strerror(errno) << std::endl;
		close(socketFd);
		return (-1);
	}

	// found 512 in /proc/sys/net/ipv4/tcp_max_syn_backlog
	if (listen(socketFd, 512) == -1) {
		std::cerr << "listen() failed: " << strerror(errno) << std::endl;
		close(socketFd);
		return (-1);
	}

	return (socketFd);
}

void	ServerManager::groupServersByEndPoint(void) {

	// Key: ["address:port", index] in _endpoints
	std::map<std::string, size_t> endpointMap;

	for (size_t i = 0; i < _servers.size(); ++i) {

		server& srv = _servers[i];

		// Check and handle multiple listen directives in a single server block
		for (size_t j = 0; j < srv.lis.size(); ++j) {

			const listenDirective& lis = srv.lis[j];

			// Formatting ip:port into key (string) -> 0.0.0.0:8080 for example
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

bool	ServerManager::configureSocket(int socketFd) {

	// allow address reuse (avoid "Address already in use" when restarting server/while developping)
	int optval = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
		return (false);
	}

	// adding O_NONBLOCK to list of existing flags
	if (fcntl(socketFd, F_SETFL, O_NONBLOCK) < 0) {
		std::cerr << "fcntl(F_SETFL, O_NONBLOCK) failed: " << strerror(errno) << std::endl;
		return (false);
	}

	return (true);
}


/* DEBUG */
void	ServerManager::printEndpoints(void) {

	std::cout << std::endl;
	for (size_t i = 0; i < _endpoints.size(); ++i) {

		const ListenEndPoint& ep = _endpoints[i];

		std::cout << "Listening on " GREEN << ep.addr << ":" << ep.port << std::endl
		<< RESET "Socket fd[" RED << ep.socketFd << RESET "]" << std::endl
		<< RED << ep.servers.size() << RESET " virtual host(s)" << std::endl;
		for (size_t j = 0; j < ep.servers.size(); ++j) {
			std::cout << "server_names: ";
			for (size_t k = 0; k < ep.servers[j]->serverName.size(); ++k) {
				std::cout << "\"" BLUE << ep.servers[j]->serverName[k] << RESET "\" ";
			}
			std::cout << "| isRunning: " << (ep.servers[j]->isRunning ? GREEN "true" : RED "false") << RESET << std::endl;
		}
		std::cout << RESET << std::endl;
	}
}

/* utils */
static uint32_t	ipv4_str_to_int(const std::string &address)
{
	uint32_t			res = 0;
	std::istringstream	iss(address);
	unsigned int		bytes;

	for (size_t i = 0; i < 4; ++i) {
		iss >> bytes;
		res |= bytes << (24 - (i * 8));

		char dot;
		iss >> dot;
	}
	return (htonl(res));
}

bool	ServerManager::isListenSocket(int fd) const {
	return (_socketToEndpoint.find(fd) != _socketToEndpoint.end());
}

/* getter */

std::vector<int>	ServerManager::getListenSocketFds(void) {

	std::vector<int>	fds;

	for (size_t i = 0; i < _endpoints.size(); ++i) {
		if (_endpoints[i].socketFd >= 0) {// not sure if I have to exclude 0 here
			fds.push_back(_endpoints[i].socketFd);
		}
	}
	return (fds);
}

std::vector<server>	ServerManager::getServers(void) {
	return (_servers);
}

globalDir ServerManager::getGlobalDir(void) {
	return (_globalDir);
}
