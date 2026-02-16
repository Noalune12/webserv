#include <netinet/in.h>
#include <sstream>

#include "EventLoop.hpp"

void	EventLoop::getClientInfo(struct sockaddr_in& addr, std::string& ip, int& port) {

	uint32_t ip_host = ntohl(addr.sin_addr.s_addr);
	std::ostringstream oss;
	for (size_t i = 0; i < 4; ++i) {
		oss << ((ip_host >> (24 - (i * 8))) & 0xFF);
		if (i < 3)
			oss << ".";
	}
	ip = oss.str();
	port = ntohs(addr.sin_port);
}

void	EventLoop::stop(void) {
	_running = false;
}

bool	EventLoop::isRunning(void) const {
	return (_running);
}

size_t	EventLoop::getConnectionCount(void) const {
	return (_connections.size());
}

std::vector<int>	EventLoop::getListenSocketFds(void) const {
	return (_serverManager.getListenSocketFds());
}

void	EventLoop::closeEpollFd(void) {
	if (_epollFd >= 0) {
		close(_epollFd);
		_epollFd = -1;
	}
}
