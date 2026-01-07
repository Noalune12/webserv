#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "EventLoop.hpp"
#include "colors.h"

EventLoop::EventLoop(ServerManager& serverManager) : _epollFd(-1), _running(false), _serverManager(serverManager), _connections() {}

EventLoop::~EventLoop() {

	std::map<int, Connection>::iterator it;

	for (it = _connections.begin(); it != _connections.end(); ++it) {
		close(it->first);
	}
	_connections.clear();

	if (_epollFd >= 0) {
		close(_epollFd);
		_epollFd = -1;
	}
}

bool	EventLoop::init(void) {

	_epollFd = epoll_create(42); // parce que pourquoi pas
	if (_epollFd < 0) {
		std::cerr << RED << "epoll_create() failed: " << strerror(errno) << RESET << std::endl;
		return (false);
	}

	std::vector<int> listenFds = _serverManager.getListenSocketFds();

	for (size_t i = 0; i < listenFds.size(); ++i) {
		if (!addToEpoll(listenFds[i], EPOLLIN)) {
			close(_epollFd);
			_epollFd = -1;
			return (false);
		}
	}

	std::cout << GREEN << "EventLoop initialized with " << listenFds.size() << " listen socket(s)" << RESET << std::endl;
	return (true);
}

void	EventLoop::run(void) {

	_running = true;
	struct epoll_event events[MAX_EVENTS];


	(void) events;


	std::cout << BLUE << "EventLoop running..." << RESET << std::endl;

	while (_running) {
		// TODO
	}

	std::cout << YELLOW << "EventLoop stopped" << RESET << std::endl;
}

// projection for signal handling
void	EventLoop::stop(void) {
	_running = false;
}

bool	EventLoop::isRunning(void) const {
	return (_running);
}

size_t	EventLoop::getConnectionCount(void) const {
	return (_connections.size());
}


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

	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
		std::cerr << "epoll_ctl(MOD) failed for fd " << fd << ": " << strerror(errno) << std::endl;
		return (false);
	}

	return (true);
}

bool	EventLoop::removeFromEpoll(int fd) {

	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		if (errno != ENOENT) { // ENOENT means the fd is not registered to the epoll instance, I don't think we should care if it happens
			std::cerr << "epoll_ctl(DEL) failed for fd " << fd << ": " << strerror(errno) << std::endl;
			return (false);
		}
	}

	return (true);
}

void	EventLoop::closeConnection(int clientFd) {
	// TODO
	(void) clientFd;
}


/* utils */
bool	EventLoop::setNonBlocking(int fd) {

	int	flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0)
		return (false);

	return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
}
