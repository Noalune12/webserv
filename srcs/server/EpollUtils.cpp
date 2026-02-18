#include <cerrno>
#include <cstring>
#include <sstream>
#include <sys/epoll.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

bool	EventLoop::addToEpoll(int fd, uint32_t events) {

	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
		if (errno != EEXIST) { // not considered an error
			std::ostringstream ss;
			ss << "epoll_ctl(ADD) failed for fd " << fd << ": " << std::strerror(errno);

			return (false);
		}
	}

	return (true);
}

// sets events (EPOLLIN | EPOLLOUT)
bool	EventLoop::modifyEpoll(int fd, uint32_t events) {

	if (_connections.find(fd) == _connections.end())
		return (false);

	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
		std::ostringstream ss;
		ss << "epoll_ctl(MOD) failed for fd " << fd << ": " << std::strerror(errno);
		Logger::error(ss.str());
		return (false);
	}

	return (true);
}

bool	EventLoop::removeFromEpoll(int fd) {

	// can pass NULL as event parameter here, school computer kernel are on a version > 2.6.9 so we will not face a bug doing it that way
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		if (errno != ENOENT) { // following the EEXIST check on addToEpoll(), not considering that as an error either
			std::ostringstream ss;
			ss << "epoll_ctl(DEL) failed for fd " << fd << ": " << std::strerror(errno);
			Logger::error(ss.str());
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

	if (client.cgi.isActive()) {
		cgiExecutor.cleanup(client.cgi, *this);
	}

	std::ostringstream	oss;
	oss << "client #" << clientFd << " disconnected";

	if (removeFromEpoll(clientFd))
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
