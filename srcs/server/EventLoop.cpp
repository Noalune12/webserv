#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/wait.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

EventLoop::EventLoop(ServerManager& serverManager) : _epollFd(-1), _running(false), _serverManager(serverManager), _connections(), _pipeToClient(), cgiExecutor() {}

EventLoop::~EventLoop() {

	std::map<int, Connection>::iterator it;
	for (it = _connections.begin(); it != _connections.end(); ++it) {
		Connection& client = it->second;
		if (client.cgi.isActive()) {
			cgiExecutor.cleanup(client.cgi, *this);
		}
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
	_epollFd = epoll_create(PROXY_AUTH_REQ);
	if (_epollFd < 0) {
		Logger::error(std::string("epoll_create() failed: ") + strerror(errno));
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
	_running = true;
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
	Connection newClient(clientFd, ip, _serverManager.getServers(), _serverManager.getGlobalDir());

	newClient.setState(IDLE);
	newClient.startTimer(IDLE, CLIENT_TIMEOUT);

	if (!addToEpoll(clientFd, EPOLLIN)) { // /!\ no EPOLLOUT here to avoid triggering epoll_wait once for each socket after creation
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
		case CGI_WRITING_BODY:
		case CGI_RUNNING:
			handleCGIClientEvent(client, clientFd, ev);
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
	if (client.getState() == CGI_WRITING_BODY) {
		cgiExecutor.handleCGIWriteEvent(client, clientFd, pipeFd, ev, *this);
	} else {
		cgiExecutor.handlePipeEvent(client, clientFd, pipeFd, ev, *this);
	}
}

void	EventLoop::handleIdle(Connection& client, int clientFd, uint32_t ev) {
	(void) ev;
	client.setState(READING_HEADERS);
	client.startTimer(1, DATA_MANAGEMENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN);
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

	if (!client.request.isCRLF(client.getBuffer()))
		return;

	client.parseRequest();

	// check if need to read body (chunked request)
	if ((client.request.chunkRemaining || client.request.multipartRemaining || client.request.remainingBody) && !client.request.err) {
		transitionToReadingBody(client, clientFd);
		return ;
	}

	// check if CGI
	if (client.request.isCgi && !client.request.err) {
		transitionToCGI(client, clientFd);
		return ;
	}

	// Normal request
	if (!client.request.err && !client.request.isCgi && !client.request.returnDirective) {
		client.request.methodHandler();
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
		client.request.chunk += client.getChunkBuffer();
		client.request.fullBody += client.getChunkBuffer();
		client.clearChunkBuffer();
		client.startTimer(2, DATA_MANAGEMENT_TIMEOUT);
	}
	if (client.request.isChunked)
		client.request.parseChunk();
	else if (client.request.isMultipart)
		client.request.parseMultipart();
	else if (client.request.remainingBody)
		client.request.checkBodySize(client.request.fullBody);

	if (!client.request.chunkRemaining && !client.request.err && !client.request.isCgi && !client.request.returnDirective \
		&& !client.request.multipartRemaining && !client.request.remainingBody) {
		client.request.methodHandler();
		client.clearChunkBuffer();
		transitionToSendingResponse(client, clientFd);
	} else if (client.request.isCgi && !client.request.err) {
		transitionToCGI(client, clientFd);
	}
	else if ((!client.request.chunkRemaining && !client.request.multipartRemaining && !client.request.remainingBody) || client.request.err) {
		client.clearChunkBuffer();
		transitionToSendingResponse(client, clientFd);
	}
}

void	EventLoop::handleCGIClientEvent(Connection& client, int clientFd, uint32_t ev) {
	if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		if (client.cgi.pid > 0) {
			kill(client.cgi.pid, SIGKILL);
		}
		cgiExecutor.cleanup(client.cgi, *this);
		closeConnection(clientFd);
		return ;
	}

	if (ev & EPOLLIN) {
		char	buf[1];
		ssize_t	n = recv(clientFd, buf, 1, MSG_PEEK | MSG_DONTWAIT); // check deeper
		if (n == 0) {
			// EOF — client closed the connection
			if (client.cgi.pid > 0)
				kill(client.cgi.pid, SIGKILL);
			cgiExecutor.cleanup(client.cgi, *this);
			closeConnection(clientFd);
			return ;
		}
	}
}

void	EventLoop::handleSendingResponse(Connection& client, int clientFd, uint32_t ev) {

	if (!(ev & EPOLLOUT)) {
		return ;
	}

	if (client.sendBuffer.empty()) {

		Response response;

		if (!client.cgi.outputBuff.empty()) {
			response.buildFromCGI(client.cgi.outputBuff, client.request);
			client.cgi.outputBuff.clear();
		} else {
			if (client.request.err && client.request.reqLocation) {
				std::string	root = client.request.reqLocation->root.empty() ? client.request.reqLocation->alias : client.request.reqLocation->root;
				client.request.findErrorPage(client.request.status, root, client.request.reqLocation->errPage);
			}
			response.buildFromRequest(client.request);
		}

		client.sendBuffer = response.prepareRawData();
		client.sendOffset = 0;

		Logger::accessLog(client.getIP(), client.request.method, client.request.uri, client.request.version, response.getStatusCode(), response.getBodySize());
	}

	size_t	remaining = client.sendBuffer.size() - client.sendOffset;
	ssize_t	bytesSent = ::send(clientFd, &client.sendBuffer[client.sendOffset], remaining, MSG_NOSIGNAL);

	if (bytesSent < 0) {
		Logger::error("Failed to send response");
		client.clearSendBuffer();
		closeConnection(clientFd);
		return ;
	}

	client.sendOffset += bytesSent;

	if (client.sendOffset < client.sendBuffer.size()) {
		client.startTimer(4, 5);
		return ;

	}

	client.clearSendBuffer();

	if (!client.request.keepAlive) {
		closeConnection(clientFd);
	} else {
		transitionToIDLE(client, clientFd);
		client.clearBuffer();
	}
}

void	EventLoop::transitionToIDLE(Connection& client, int clientFd) {
	client.request.clearPreviousRequest();
	client.setBuffer("");
	client.setState(IDLE);
	client.startTimer(0, CLIENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN);
}

void	EventLoop::transitionToReadingBody(Connection& client, int clientFd) {
	client.setState(READING_BODY);
	client.startTimer(2, DATA_MANAGEMENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN);
}

void	EventLoop::transitionToCGI(Connection& client, int clientFd) {
	if (cgiExecutor.start(client, clientFd, *this)) {
		if (client.cgi.pipeIn[1] != -1) {
			client.setState(CGI_WRITING_BODY);
			client.startTimer(3, CGI_TIMEOUT);
		} else {
			client.setState(CGI_RUNNING);
			client.startTimer(3, CGI_TIMEOUT);
		}
	} else {
		client.request.err = true;
		client.request.status = 500;
		transitionToSendingResponse(client, clientFd);
	}
}

void	EventLoop::transitionToSendingResponse(Connection& client, int clientFd) {
	client.setState(SENDING_RESPONSE);
	client.startTimer(4, DATA_MANAGEMENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN | EPOLLOUT); // both EPOLLIN and EPOLLOUT here to make sure no new incoming data will not wake the event loop because EPOLLIN is no longer set on the socket.
}

size_t	EventLoop::readFromClient(int clientFd, Connection& client) {

	char	buffer[8192];
	ssize_t	bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead == -1) {
		Logger::error("recv failed");
	}

	if (bytesRead > 0) {
		if (client.getState() == READING_BODY) {
			std::string chunk(buffer, bytesRead);
			client.setChunkBuffer(chunk);
		}
		std::string	currentBuffer = client.getBuffer();
		currentBuffer.append(buffer, bytesRead);
		client.setBuffer(currentBuffer);
		return (static_cast<size_t>(bytesRead));
	}
	return (0);
}

bool	EventLoop::checkEpollErrors(uint32_t ev, int clientFd) {

	if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		closeConnection(clientFd);
		return (true);
	}

	return (false);
}

bool	EventLoop::checkTimeout(Connection& client, int clientFd) {

	int	timerIdx = getActiveTimer(client.getState());
	if (timerIdx >= 0 && client.isTimedOut(timerIdx)) {
		// if client is in IDLE mode, do not send anything just close,
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
