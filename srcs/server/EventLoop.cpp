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
#include <sstream>

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

	_epollFd = epoll_create(42); // parce que pourquoi pas (go mettre un esther egg)
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

	std::cout << BLUE << "EventLoop running..." << RESET << std::endl;

	while (_running) {
		int	nEvents = epoll_wait(_epollFd, events, MAX_EVENTS, 10000); // define for timeout ?

		if (nEvents < 0) {
			if (errno == EINTR) // errno error for signal interruption
				continue ;
			std::cerr << "epoll_wait() failed: " << strerror(errno) << std::endl;
			break ;
		}

		// main loop, will dispatch the sockets to specific handlers
		for (int i = 0; i < nEvents; ++i) {

			int			fd = events[i].data.fd;
			uint32_t	ev = events[i].events;

			(void) ev;

			// accept + client informations storage
			if (_serverManager.isListenSocket(fd))
				acceptConnection(fd);
			else
				handleClientTest(fd, ev);
			// else if () {
			// } // client ?
			// else {
			// } // cgi ?
		}
	}
	std::cout << YELLOW << "EventLoop stopped" << RESET << std::endl;
}

void	EventLoop::handleClientTest(int clientFd, uint32_t ev) {

	if (ev & EPOLLIN) {
		std::cout << "TEST: reading data from client socket" << std::endl;
		char	buffer[1024];
		std::memset(buffer, 0, 1024);
		ssize_t	bytes = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
		if (bytes <= 0) {
			if (bytes == -1) {
				std::cout << "recv failed: " << strerror(errno) << std::endl;
			} else {
				std::cout << "removing client fd from epoll interest list" << std::endl;
			}
			removeFromEpoll(clientFd);
			// close(clientFd);
			std::cout << "did not close fd tho" << std::endl;
			return ;
		}
		std::string req = buffer;
		buffer[bytes] = '\0';
		std::cout << "Message from fd[" << clientFd << "]:\n" << buffer << std::endl;
		modifyEpoll(clientFd, EPOLLOUT);
	} else {
		send400(clientFd);
		modifyEpoll(clientFd, EPOLLIN);
	}

}

void	EventLoop::acceptConnection(int listenFd) {

	struct sockaddr_in	clientAddr;
	socklen_t			clientLen = sizeof(clientAddr);

	int	clientFd = accept(listenFd, (struct sockaddr *)&clientAddr, &clientLen);
	if (clientFd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { // if one of those appear then we do not consider it an error, not printing anything
			std::cerr << "accept() failed on fd " << listenFd << ": " << strerror(errno) << std::endl;
		}
		return ;
	}

	if (!setNonBlocking(clientFd)) {
		close(clientFd);
		return ;
	}

	Connection newClient(clientFd); // pretty much sure there are other thing we can already fill in here

	if (!addToEpoll(clientFd, EPOLLIN)) {
		close(clientFd);
		return ;
	}

	_connections[clientFd] = newClient;

	std::cout << BLUE << "New connection fd[" << clientFd << "]" << RESET << std::endl;
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


void EventLoop::send400(int clientFd) {
    // find if error page 400 in config file
    std::string body =
        "<html>\n"
        "<head><title>400 Bad Request</title></head>\n"
        "<body><h1>400 Bad Request</h1>\n"
        "<p>Your request was malformed.</p>\n"
        "</body>\n"
        "</html>\n";

    std::stringstream ss;
    ss << body.size();
    std::string bodySize = ss.str();

    std::string response =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + bodySize + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;

    send(clientFd, response.c_str(), response.size(), 0);

}
