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

#include "colors.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

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

	Logger::notice("using the \"epoll\" event method");
	_epollFd = epoll_create(42); // parce que pourquoi pas (go mettre un esther egg)
	if (_epollFd < 0) {
		Logger::error(std::string("epoll_create() failed:") + strerror(errno));
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
	std::ostringstream oss;
	oss << "eventLoop initialized with " << listenFds.size() << " listen socket(s)";
	Logger::debug(oss.str());
	return (true);
}

void	EventLoop::checkTimeouts(void) {

	std::vector<int>	timedOut;

	std::map<int, Connection>::iterator	it;
	for (it = _connections.begin(); it != _connections.end(); ++it) {
		if (it->second.isTimedOut(CLIENT_TIMEOUT)) {
			timedOut.push_back(it->first);
		}
	}

	for (size_t i = 0; i < timedOut.size(); ++i) {

		int	clientFd = timedOut[i];

		std::ostringstream	oss;
		oss << "client #" << clientFd << " timeout, closing";
		Logger::warn(oss.str());
		// send408 -> timeout error
		// sendTimeout(clientFd);
		closeConnection(clientFd);
	}
}

int	EventLoop::calculateEpollTimeout(void) {
	if (_connections.empty())
		return (5);

	int min_sec = 5;
	std::map<int, Connection>::const_iterator	it;
	for (it = _connections.begin(); it != _connections.end(); ++it) {
		long	rem = it->second.secondsToClosestTimeout();
		if (rem < min_sec)
			min_sec = static_cast<int>(rem);
	}
	return (min_sec);
}

void	EventLoop::run(void) {

	_running = true;
	struct epoll_event events[MAX_EVENTS];

	Logger::notice("eventLoop running...");

	while (_running) {
		int	timeout_sec = calculateEpollTimeout();
		int	nEvents = epoll_wait(_epollFd, events, MAX_EVENTS, timeout_sec * 1000); // define for timeout (OUI) ? I'm not decided yet on the value here I need to think about it a bit deeper... But if we set it to -1 maybe we dont need a define
		if (nEvents < 0) {
			if (errno == EINTR) // errno error for signal interruption
				continue ;
			Logger::error(std::string("epoll_wait() failed: ") + strerror(errno));
			break ;
		}

		checkTimeouts();

		// main loop, will dispatch the sockets to specific handlers
		for (int i = 0; i < nEvents; ++i) {

			int			fd = events[i].data.fd;
			uint32_t	ev = events[i].events;

			// accept + client informations storage
			if (_serverManager.isListenSocket(fd))
				acceptConnection(fd);
			else
				handleClientTest(fd, ev);
			// else if () {
				// } // cgi pipe ?
			// else {
				// } // client ?
		}
	}
	Logger::debug("eventLoop stopped"); // will have to be deleted since we get there if the server stops, and the only way to stop it is to send a SIGINT signal to the server. It gets printed after the signalHandling messages
}

void	EventLoop::handleClientTest(int clientFd, uint32_t ev) {

	Connection&	client = _connections[clientFd];
	(void) client;

    if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) { // remove the if/elseif below after we discussed about the issues I faced
		if (ev & EPOLLERR) {
			std::cerr << RED "EPOLLERR - fd[" << clientFd << "]" RESET << std::endl;
		} else if (ev & EPOLLHUP) {
			std::cerr << RED "EPOLLHUP - fd[" << clientFd << "]" RESET << std::endl;
		} else if (ev & EPOLLRDHUP) {
			std::cerr << RED "EPOLLRDHUP - fd[" << clientFd << "]" RESET << std::endl;
		}
        removeFromEpoll(clientFd);
        close(clientFd);
		_connections.erase(clientFd);
        return ;
    }

    if (ev & EPOLLIN) {
        tempCall(clientFd);
		modifyEpoll(clientFd, EPOLLOUT);
    }

    if (ev & EPOLLOUT) {
        // send400(clientFd);
		send505exemple(clientFd);
		// closeConnection(clientFd);
		/* pour plus tard
		if (_connections._keepAlive) {
			_connections._state = READING_REQUEST;
			modifyEpoll(clientFd, EPOLLIN);
		} else {
		 	closeConnection(clientFd);
		} */
    }
}

void	EventLoop::tempCall(int clientFd) {
		// static int a = 0;
		// std::cout << "TEST: reading data from client socket -> number of call: " << ++a << std::endl;
		char	buf[10];
		std::memset(buf, 0, 10);
		std::string buffer;
		ssize_t	bytes;
		while ((bytes = recv(clientFd, buf, sizeof(buf), 0)) > 0) { // not sure its completely done, not checking errno, only logging it
			if (bytes <= 0) {
				if (bytes == -1) {
					std::cout << "recv failed: " << strerror(errno) << std::endl;
				}
				return ;
			}
			buffer += std::string(buf, bytes);
		}
		// std::string req = buffer;
		// std::cout << YELLOW "Message from fd[" << clientFd << "]:\n" RESET << buffer;
}

void	EventLoop::acceptConnection(int listenFd) {

	struct sockaddr_in	clientAddr;
	socklen_t			clientLen = sizeof(clientAddr);

	int	clientFd = accept(listenFd, (struct sockaddr *)&clientAddr, &clientLen);
	if (clientFd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { // if one of those appear then we do not consider it an error, not printing anything
			std::cerr << "accept() failed on fd[" << listenFd << "]: " << strerror(errno) << std::endl;
		}
		return ;
	}

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
		close(clientFd);
		return ;
	}

	std::string clientIp;
	int			clientPort;
	getClientInfo(clientAddr, clientIp, clientPort);
	Connection newClient(clientFd, clientIp, clientPort);

	if (!addToEpoll(clientFd, EPOLLIN)) { // not sure if I HAVE to add EPOLLIN and EPOLLOUT here
		close(clientFd);
		return ;
	}

	_connections[clientFd] = newClient;

	std::ostringstream	oss;
	oss << "new client #" << clientFd << " from " << clientIp << ":" << clientPort;
	Logger::notice(oss.str());
}

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
	removeFromEpoll(clientFd);
	close(clientFd);
	_connections.erase(clientFd);
	std::ostringstream	oss;
	oss << "client #" << clientFd << " disconnected";
	Logger::notice(oss.str());
}


/* utils */
// bool	EventLoop::setNonBlocking(int fd) {
// 	return (fcntl(fd, F_SETFL, O_NONBLOCK) >= 0);
// }


void EventLoop::send400(int clientFd) {
	std::cout << GREEN "Sending 400 response to fd[" << clientFd << "]" RESET << std::endl;

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

	send(clientFd, response.c_str(), response.size(), 0); // flags no use ? MSG_NOSIGNAL | MSG_DONTWAIT | also MSG_OOB

	Connection& client = _connections[clientFd];

	Logger::accessLog(client.getIP(), "method", "uri", "version", -1, body.size());
	// std::cout << GREEN "Sent " << sent << " bytes to fd[" << clientFd << "]" RESET << std::endl;
}


void	EventLoop::send505exemple(int clientFd) {

	std::string body =
		"<html>\n"
		"<html lang=\"en\">\n"
		"<head>\n"
		"<title>505 HTTP Version Not Supported</title>\n"
		"</head>\n"
		"<body>\n"
		"<h1>505 HTTP Version Not Supported</h1>\n"
		"<p>If this problem persists, please <a href=\"https://example.com/support\">contact support</a>.</p>\n"
		"<p>Server logs contain details of this error with request ID: ABC-123.</p>\n"
		"</body>\n"
		"</html>\n";

		std::stringstream	ss;
		ss << body.size();
		std::string bodySize = ss.str();

	std::string res =
		"HTTP/1.1 505 HTTP Version Not Supported\r\n"
		"Content-Type: text/html;\r\n"
		"Content-Length: " + bodySize + "\r\n"
		"\r\n" +
		body;

	send(clientFd, res.c_str(), res.size(), 0);
	// std::cout << GREEN "Sent " << sent << " bytes to fd[" << clientFd << "]" RESET << std::endl;
}
