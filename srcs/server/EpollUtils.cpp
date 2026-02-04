#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "EventLoop.hpp"
#include "Logger.hpp"

bool	EventLoop::addToEpoll(int fd, uint32_t events) {

	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
		if (errno != EEXIST) { // I think we want to skip this case as it doesn't cause any trouble (does it ?)
			std::cerr << "epoll_ctl(ADD) failed for fd " << fd << ": " << strerror(errno) << std::endl;
			return (false);
		}
	}

	return (true);
}

// works for EPOLLIN | EPOLLOUT, can also set them both at the same time (see if this we have the use -> discussed with lthan)
bool	EventLoop::modifyEpoll(int fd, uint32_t events) {

	if (_connections.find(fd) == _connections.end())
		return (false);

	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
		int mypid = getpid();
		std::cout << mypid << std::endl;
		std::cerr << "epoll_ctl(MOD) failed for fd " << fd << ": " << strerror(errno) << std::endl;
		return (false);
	}

	return (true);
}

bool	EventLoop::removeFromEpoll(int fd) {

	// can pass NULL as event parameter here, school computer kernel are on a version > 2.6.9 so we will not face a bug doing it that way (might want to defend it with other words lol)
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		if (errno != ENOENT) { // ENOENT means the fd is not registered to the epoll instance, I don't think we should care if it happens
			std::cerr << "epoll_ctl(DEL) failed for fd " << fd << ": " << strerror(errno) << std::endl;
			return (false);
		}
	}

	return (true);
}

void	EventLoop::closeConnection(int clientFd) {

	std::map<int, Connection>::iterator it = _connections.find(clientFd);
	if (it == _connections.end())
		return ;

	Connection& client = it->second;

	if (client._cgi.isActive()) {
		_cgiExecutor.cleanup(client._cgi);
	}

	std::ostringstream	oss;
	oss << "client #" << clientFd << " disconnected";

	removeFromEpoll(clientFd); // need to check the return value of this function depending on the cases
	Logger::notice(oss.str());

	close(clientFd);
	_connections.erase(it);
}

void	EventLoop::registerPipe(int pipeFd, int clientFd) {
	_pipeToClient[pipeFd] = clientFd;
}

void	EventLoop::unregisterPipe(int pipeFd) {
	_pipeToClient.erase(pipeFd);
}
