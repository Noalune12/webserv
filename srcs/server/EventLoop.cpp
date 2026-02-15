#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/wait.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

EventLoop::EventLoop(ServerManager& serverManager) : _epollFd(-1), _running(false), _serverManager(serverManager), _connections(), _pipeToClient(), _cgiExecutor() {}

EventLoop::~EventLoop() {

	std::map<int, Connection>::iterator it;
	for (it = _connections.begin(); it != _connections.end(); ++it) {
		Connection& client = it->second;
		if (client._cgi.isActive()) {
			_cgiExecutor.cleanup(client._cgi, *this);
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
	Connection newClient(clientFd, ip, _serverManager.getServers(), _serverManager.getGlobalDir());

	newClient.setState(IDLE);
	newClient.startTimer(IDLE, CLIENT_TIMEOUT);

	if (!addToEpoll(clientFd, EPOLLIN)) { // no EPOLLOUT here to avoid triggering epoll_wait once for each socket after creation
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
			handleCGIRunning(client, clientFd, ev);
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
	if (client.getState() == CGI_WRITING_BODY) {
		_cgiExecutor.handleCGIWriteEvent(client, clientFd, pipeFd, ev, *this);
	} else {
		_cgiExecutor.handlePipeEvent(client, clientFd, pipeFd, ev, *this);
	}
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

	if (!client._request.isCRLF(client.getBuffer()))
		return;

	client.parseRequest();

	// check if need to read body (chunked request)
	if (client._request.chunkRemaining || client._request._multipartRemaining || client._request._remainingBody) {
		transitionToReadingBody(client, clientFd);
		return ;
	}

	// check if CGI
	if (client._request._cgi && !client._request.err) {
		transitionToCGI(client, clientFd);
		return ;
	}

	// Normal request
	if (!client._request.err && !client._request._cgi && !client._request._return) {
		client._request.methodHandler();
	}

	transitionToSendingResponse(client, clientFd);
}

void	EventLoop::handleReadingBody(Connection& client, int clientFd, uint32_t ev) {

	if (ev & EPOLLIN) {
		Logger::debug("READING_BODY state");
		size_t	bytesRead = readFromClient(clientFd, client);
		if (bytesRead == 0) {
			closeConnection(clientFd);
			return ;
		}
		client._request._chunk += client.getChunkBuffer();
		client._request._fullBody += client.getChunkBuffer();
		client.clearChunkBuffer();
		client.startTimer(2, CLIENT_TIMEOUT - 2);
	}
	if (client._request._isChunked)
		client._request.parseChunk();
	else if (client._request._isMultipart)
		client._request.parseMultipart();
	else if (client._request._remainingBody)
		client._request.parseBody();

	if (!client._request.chunkRemaining && !client._request.err && !client._request._cgi && !client._request._return \
		&& !client._request._multipartRemaining && !client._request._remainingBody) {
		client._request.methodHandler();
		client.clearChunkBuffer();
		transitionToSendingResponse(client, clientFd);
	} else if (client._request._cgi && !client._request.err) {
		transitionToCGI(client, clientFd);
	}
	else if ((!client._request.chunkRemaining && !client._request._multipartRemaining && !client._request._remainingBody) || client._request.err) {
		client.clearChunkBuffer();
		transitionToSendingResponse(client, clientFd);
	}
}

void	EventLoop::handleCGIRunning(Connection& client, int clientFd, uint32_t ev) {
	if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		if (client._cgi.pid > 0) {
			kill(client._cgi.pid, SIGKILL);
		}
		Logger::debug("Client disconnection while CGI running");
		// client._request.status = 500; -> sending 500 or 504 ?
		_cgiExecutor.cleanup(client._cgi, *this);
		closeConnection(clientFd);
		return ;
	}

	Logger::debug("CGI_RUNNING: waiting for CGI to complete");
}

void	EventLoop::handleSendingResponse(Connection& client, int clientFd, uint32_t ev) {

	if (!(ev & EPOLLOUT)) {
		return ;
	}

	if (client._sendBuffer.empty()) {

		Response response;

		response.debugPrintRequestData(client._request);

		if (!client._cgi.outputBuff.empty()) {
			Logger::debug("Building CGI response");
			response.buildFromCGI(client._cgi.outputBuff, client._request);
			client._cgi.outputBuff.clear();
		} else {
			response.buildFromRequest(client._request);
		}

		client._sendBuffer = response.prepareRawData();
		client._sendOffset = 0;

		Logger::accessLog(client.getIP(), client._request._method, client._request._uri, client._request._version, response.getStatusCode(), response.getBodySize());
	}

	size_t	remaining = client._sendBuffer.size() - client._sendOffset;
	ssize_t	bytesSent = ::send(clientFd, &client._sendBuffer[client._sendOffset], remaining, MSG_NOSIGNAL);

	if (bytesSent < 0) {
		Logger::error("Failed to send response");
		client.clearSendBuffer();
		closeConnection(clientFd);
		return ;
	}

	client._sendOffset += bytesSent;

	if (client._sendOffset < client._sendBuffer.size())
		return ;

	client.clearSendBuffer();

	if (!client._request._keepAlive) {
		closeConnection(clientFd);
	} else {
		transitionToIDLE(client, clientFd);
	}
}

void	EventLoop::transitionToIDLE(Connection& client, int clientFd) {
	client._request.clearPreviousRequest();
	client.setBuffer("");
	client.setState(IDLE);
	client.startTimer(0, CLIENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN);
	Logger::debug("-> IDLE (keep-alive)");
}

void	EventLoop::transitionToReadingBody(Connection& client, int clientFd) {
	client.setState(READING_BODY);
	client.startTimer(2, CLIENT_TIMEOUT - 4); // need explaination on the -4
	modifyEpoll(clientFd, EPOLLIN);
	Logger::debug("-> READING_BODY (chunked)");
}

void	EventLoop::transitionToCGI(Connection& client, int clientFd) {
	if (_cgiExecutor.start(client, clientFd, *this)) {
		if (client._cgi.pipeIn[1] != -1) {
			client.setState(CGI_WRITING_BODY);
			client.startTimer(3, CGI_TIMEOUT);
		} else {
			client.setState(CGI_RUNNING);
			client.startTimer(3, CGI_TIMEOUT);
			Logger::debug("-> CGI_RUNNING");
		}
	} else {
		client._request.err = true;
		client._request.status = 500;
		transitionToSendingResponse(client, clientFd);
	}
}

void	EventLoop::transitionToSendingResponse(Connection& client, int clientFd) {
	client.setState(SENDING_RESPONSE);
	client.startTimer(4, CLIENT_TIMEOUT);
	modifyEpoll(clientFd, EPOLLIN | EPOLLOUT); // both EPOLLIN and EPOLLOUT here to make sure no new incoming data will not wake the event loop because EPOLLIN is no longer set on the socket.
	Logger::debug("-> SENDING_RESPONSE");
}

size_t	EventLoop::readFromClient(int clientFd, Connection& client) {

	char	buffer[8192];
	ssize_t	bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead == -1) {
		std::cerr << "recv failed: " << strerror(errno) << std::endl; // not checking errno, only logging it for now (this check might be deleted)
	}

	if (bytesRead > 0) {
		if (client.getState() == READING_BODY) {
			// buffer[bytesRead] = '\0';
			std::string chunk(buffer, bytesRead);
			// std::cout << "CHUNK BUFFER WHEN READING BODY = " << buffer << std::endl;
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
		if (ev & EPOLLERR) {
			std::cerr << RED "EPOLLERR - fd[" << clientFd << "]" RESET << std::endl;
		} else if (ev & EPOLLHUP) {
			std::cerr << RED "EPOLLHUP - fd[" << clientFd << "]" RESET << std::endl;
		} else if (ev & EPOLLRDHUP) {
			std::cerr << RED "EPOLLRDHUP - fd[" << clientFd << "]" RESET << std::endl;
		}
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
		// if client is in IDLE mode, do not send anything just close,
		// (Note: Some servers will shut down a connection without sending this message.
		// https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status/408)
		// otherwise we will send a 408 Request Timeout
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
