#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

EventLoop::EventLoop(ServerManager& serverManager) : _epollFd(-1), _running(false), _serverManager(serverManager), _connections(), _pipeToClient(), _cgiExecutor(), _responseBuilder(), _responseSender() {}

EventLoop::~EventLoop() {

	if (_epollFd >= 0) {
		close(_epollFd);
		_epollFd = -1;
	}

		std::map<int, Connection>::iterator it;

	for (it = _connections.begin(); it != _connections.end(); ++it) {
		close(it->first);
	}
	_connections.clear();

	// // cgi fds cleanup ?
}

bool	EventLoop::init(void) {

	Logger::notice("using the \"epoll\" event method");
	_epollFd = epoll_create(PROXY_AUTH_REQ);
	if (_epollFd < 0) {
		Logger::error(std::string("epoll_create() failed: ") + strerror(errno));
		return (false);
	}

	std::vector<int> listenFds = _serverManager.getListenSocketFds();
	for (size_t i = 0; i < listenFds.size(); ++i) {
		if (!addToEpoll(listenFds[i], EPOLLIN)) {
			// close(_epollFd);
			// _epollFd = -1;
			return (false);
		}
	}
	_running = true;
	std::ostringstream oss;
	oss << "eventLoop initialized with " << listenFds.size() << " listen socket(s)";
	Logger::debug(oss.str());
	return (true);
}

void	EventLoop::run(void) {

	struct epoll_event events[MAX_EVENTS];

	Logger::notice("eventLoop running...");

	while (_running) {
		int timeout = calculateEpollTimeout() * 1000;
		int nfds = epoll_wait(_epollFd, events, MAX_EVENTS, timeout);

		if (nfds == -1) {
			if (errno == EINTR) // errno code for signal interruption
				continue ;
			Logger::error(std::string("epoll_wait() failed: ") + strerror(errno));
			break ;
		}

		checkTimeouts();

		for (int i = 0; i < nfds; ++i) {

			int			fd = events[i].data.fd;
			uint32_t	ev = events[i].events;

			if (_serverManager.isListenSocket(fd)) {
				acceptConnection(fd);
			}
			else if (_pipeToClient.find(fd) != _pipeToClient.end()) {
				handleCGIPipeEvent(fd, ev);
			}
			else {
				handleClientEvent(fd, ev);
			}
		}
	}
}

void	EventLoop::acceptConnection(int listenFd) {

	struct sockaddr_in	clientAddr;
	socklen_t			addrLen = sizeof(clientAddr);

	int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &addrLen);
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

	std::string	ip;
	int			port;
	getClientInfo(clientAddr, ip, port);
	Connection newClient(clientFd, ip, port, _serverManager.getServers(), _serverManager.getGlobalDir());

	newClient.setState(IDLE);
	newClient.startTimer(IDLE, CLIENT_TIMEOUT);

	if (!addToEpoll(clientFd, EPOLLIN)) { // not sure if I HAVE to add EPOLLIN and EPOLLOUT here
		close(clientFd);
		return ;
	}

	_connections[clientFd] = newClient;

	std::ostringstream	oss;
	oss << "new client #" << clientFd << " from " << ip << ":" << port;
	Logger::notice(oss.str());
}

void	EventLoop::handleClientEvent(int clientFd, uint32_t ev) {

	std::map<int, Connection>::iterator	it = _connections.find(clientFd);
	if (it == _connections.end())
		return ;

	Connection& client = it->second;

	if (checkEpollErrors(ev, clientFd))
		return ;

	if (checkTimeout(client, clientFd))
		return ;

	switch (client.getState()) {
		case IDLE:
			handleIdle(client, clientFd, ev);
			break ;
		case READING_HEADERS:
			handleReadingHeaders(client, clientFd, ev);
			break ;
		case READING_BODY:
			handleReadingBody(client, clientFd, ev);
			break ;
		case CGI_RUNNING:
			handleCGIRunning(client, clientFd, ev);
			break ;
		case SENDING_RESPONSE:
			handleSendingResponse(client, clientFd, ev);
			break ;
		default:
			break ;
	}
}

void	EventLoop::handleCGIPipeEvent(int pipeFd, uint32_t ev) {

	std::map<int, int>::iterator	it = _pipeToClient.find(pipeFd);
	if (it == _pipeToClient.end())
		return ;

	int	clientFd = it->second;
	std::map<int, Connection>::iterator	clientIt = _connections.find(clientFd);

	if (clientIt == _connections.end()) {
		removeFromEpoll(pipeFd);
		close(pipeFd);
		_pipeToClient.erase(it);
		return ;
	}

	Connection& client = clientIt->second;
	_cgiExecutor.handlePipeEvent(client, clientFd, pipeFd, ev, *this);
}

void	EventLoop::handleIdle(Connection& client, int clientFd, uint32_t ev) {
	(void) ev;
	client.setState(READING_HEADERS);
	client.startTimer(1, CLIENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN);
	Logger::debug("IDLE -> READING_HEADERS");
}

void	EventLoop::handleReadingHeaders(Connection& client, int clientFd, uint32_t ev) {

	if (ev & EPOLLIN) {
		size_t	bytesRead = readFromClient(clientFd, client);
		if (bytesRead == 0) {
			closeConnection(clientFd);
			return ;
		}
	}

	if (client.getBuffer().empty()) {
		closeConnection(clientFd);
		return ;
	}

	client.parseRequest();

	// check if need to read body (chunked request)
	if (client._request.chunkRemaining) {
		transitionToReadingBody(client, clientFd);
		return ;
	}

	// check if CGI
	if (client._request._cgi && !client._request.err) {
		transitionToCGI(client, clientFd);
		return ;
	}

	// Normal request
	if (!client._request.err && !client._request._cgi) {
		client._request.methodHandler();
	}

	transitionToSendingResponse(client, clientFd);
}

void	EventLoop::handleReadingBody(Connection& client, int clientFd, uint32_t ev) {

	if (ev & EPOLLIN) {
		size_t	bytesRead = readFromClient(clientFd, client);
		if (bytesRead == 0) {
			closeConnection(clientFd);
			return ;
		}
		client._request._chunk += client.getBuffer();
		client.startTimer(2, CLIENT_TIMEOUT - 4);
	}

	client._request.parseChunk();

	if (!client._request.chunkRemaining) {
		if (client._request._cgi && !client._request.err) {
			transitionToCGI(client, clientFd);
		} else if (!client._request.err) {
			client._request.methodHandler();
			transitionToSendingResponse(client, clientFd);
		} else {
			transitionToSendingResponse(client, clientFd);
		}
	} else if (client._request.err) {
		transitionToSendingResponse(client, clientFd);
	}
}

void	EventLoop::handleCGIRunning(Connection& client, int clientFd, uint32_t ev) {
	if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		if (client._cgi.pid > 0) {
			kill(client._cgi.pid, SIGKILL);
		}
		Logger::debug("Client disconnection while CGI running");
		_cgiExecutor.cleanup(client._cgi);
		closeConnection(clientFd);
		return ;
	}

	Logger::debug("CGI_RUNNING: waiting for CGI to complete");
}

void	EventLoop::handleSendingResponse(Connection& client, int clientFd, uint32_t ev) {

	if (!(ev & EPOLLOUT)) {
		return ;
	}

	Response response;

	if (!client._cgi.outputBuff.empty()) {
		Logger::debug("Building CGI response");
		response = _responseBuilder.buildFromCGI(client._cgi.outputBuff, client._request);
		client._cgi.outputBuff.clear();
	} else {
		response = _responseBuilder.buildFromRequest(client._request);
	}

	ssize_t bytesSent = _responseSender.send(clientFd, response);

	if (bytesSent < 0) {
		Logger::error("Failed to send response");
		closeConnection(clientFd);
		return ;
	}

	Logger::accessLog(client.getIP(), client._request._method, client._request._uri, "HTTP/1.1", response._statusCode, response._body.size());

	if (!client._request._keepAlive) {
		closeConnection(clientFd);
	} else {
		transitionToIDLE(client, clientFd);
	}
}

void	EventLoop::transitionToIDLE(Connection& client, int clientFd) {
	client.setState(IDLE);
	client.startTimer(0, CLIENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN);
	Logger::debug("-> IDLE");
}

void	EventLoop::transitionToReadingBody(Connection& client, int clientFd) {
	client.setState(READING_BODY);
	client.startTimer(2, CLIENT_TIMEOUT - 4);
	modifyEpoll(clientFd, EPOLLIN);
	Logger::debug("-> READING_BODY (chunked)");
}

void	EventLoop::transitionToCGI(Connection& client, int clientFd) {
	if (_cgiExecutor.start(client, clientFd, *this)) {
		client.setState(CGI_RUNNING);
		client.startTimer(3, CGI_TIMEOUT);
		Logger::debug("-> CGI_RUNNING");
	} else {
		client._request.err = true;
		client._request.status = 500;
		transitionToSendingResponse(client, clientFd);
	}
}

void	EventLoop::transitionToSendingResponse(Connection& client, int clientFd) {
	client.setState(SENDING_RESPONSE);
	client.startTimer(4, CLIENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLOUT);
	Logger::debug("-> SENDING_RESPONSE");
}

size_t	EventLoop::readFromClient(int clientFd, Connection& client) {

	char	buffer[4096];
	ssize_t	bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

	if (bytesRead > 0) {
		client.setBuffer(std::string(buffer, bytesRead));
		return (static_cast<size_t>(bytesRead));
	}

	return (0);
}

bool	EventLoop::checkEpollErrors(uint32_t ev, int clientFd) {

	if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		Logger::debug("Epoll error");
		closeConnection(clientFd);
		return (true);
	}

	return (false);
}

bool	EventLoop::checkTimeout(Connection& client, int clientFd) {

	int	timerIdx = getActiveTimer(client.getState());
	if (timerIdx >= 0 && client.isTimedOut(timerIdx)) {
		Logger::warn("Timeout");
		Logger::error(std::string("epoll_wait() failed: ") + strerror(errno));
		closeConnection(clientFd);
		return (true);
	}
	return (false);
}

std::vector<int>	EventLoop::getAllClientFds(void) const {
	std::vector<int> fds;
	std::map<int, Connection>::const_iterator it;
	for (it = _connections.begin(); it != _connections.end(); ++it) {
		fds.push_back(it->first);
	}
	return (fds);
}
