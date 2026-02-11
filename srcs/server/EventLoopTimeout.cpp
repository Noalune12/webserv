#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <climits>
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
#include "StatusCodes.hpp"

void	EventLoop::checkTimeouts(void) {

	std::vector<int>					timedOut;
	std::map<int, Connection>::iterator	it;

	for (it = _connections.begin(); it != _connections.end(); ++it) {
		Connection& client = it->second;
		int activeTimer = getActiveTimer(client.getState());

		if (activeTimer >= 0 && client.isTimedOut(activeTimer)) {
			timedOut.push_back(it->first);
		}
	}

	for (size_t i = 0; i < timedOut.size(); ++i) {

		int clientFd = timedOut[i];

		it = _connections.find(clientFd);
		if (it == _connections.end())
			continue ;

		Connection&			client = it->second;

		if (client.getState() == READING_BODY) {
			Logger::warn("Body read timeout, sending 408");
			client._request.err = true;
			client._request.status = 408;
			transitionToSendingResponse(client, clientFd);
			continue ;
		}

		if (client.getState() == CGI_WRITING_BODY || client.getState() == CGI_RUNNING) {
			Logger::warn("CGI timeout, killing process");
			if (client._cgi.pid > 0) {
				kill(client._cgi.pid, SIGKILL);
			}
			_cgiExecutor.cleanup(client._cgi, *this);
			client._request.err = true;
			client._request.status = 504;
			transitionToSendingResponse(client, clientFd);
			continue ;
		}

		Logger::warn("Client timeout, closing connection");
		closeConnection(clientFd);
	}
}

int	EventLoop::getActiveTimer(ConnectionState s) {
	switch (s) {
		case IDLE:
			return (0);
		case READING_HEADERS:
			return (1);
		case READING_BODY:
			return (2);
		case CGI_WRITING_BODY:
			return (3);
		case CGI_RUNNING:
			return (3);
		case SENDING_RESPONSE:
			return (4);
		default:
			return (-1);
	}
}

int	EventLoop::calculateEpollTimeout(void) {

	if (_connections.empty())
		return (5);

	int min_sec = INT_MAX;

	std::map<int, Connection>::const_iterator it;
	for (it = _connections.begin(); it != _connections.end(); ++it) {

		long rem = it->second.secondsToClosestTimeout();
		if (rem > 0 && static_cast<int>(rem) < min_sec) {
			min_sec = static_cast<int>(rem);
		}
	}
	if (min_sec == INT_MAX)
		return (5);

	return (std::max(1, min_sec));
}
