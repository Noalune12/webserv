#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
// #include <stdlib.h>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"
#include "StatusCodes.hpp"

void	EventLoop::checkTimeouts(void) {

	std::vector<int>	timedOut;
	std::map<int, Connection>::iterator	it;

	for (it = _connections.begin(); it != _connections.end(); ++it) {
		Connection& client = it->second;
		if (it->second.getState() == CLOSED)
			continue ;
		int	active_timer = getActiveTimer(client.getState());

		if (active_timer >= 0 && client.isTimedOut(active_timer)) {
			timedOut.push_back(it->first);
		}
	}

	for (size_t i = 0; i < timedOut.size(); ++i) {

		int	clientFd = timedOut[i];

		std::ostringstream	oss;
		Connection& client = _connections[clientFd];
		if (client.getState() == READING_BODY) {
			std::ostringstream	oss;
			oss << "Body read timeout for client #" << clientFd << ", sending 408";
			Logger::warn(oss.str());

			client._request.err = true;
			client._request.status = 408;
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, CLIENT_TIMEOUT);
			modifyEpoll(clientFd, EPOLLOUT);
			continue ;
		}

		if (client.getState() == CGI_RUNNING) {
			oss << "CGI timeout for client #" << clientFd << ", killing process";
			Logger::warn(oss.str());

			if (client._cgi.pid > 0) {
				kill(client._cgi.pid, SIGKILL);
			}
			cleanupCGI(clientFd);
			client._request.err = true;
			client._request.status = 504;
			client.setState(SENDING_RESPONSE);
			modifyEpoll(clientFd, EPOLLOUT);
			continue ;
		}

		oss << "client #" << clientFd << " timeout, closing";
		Logger::warn(oss.str());
		// send408 -> timeout error
		// sendTimeout(clientFd);
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

	int min_sec = 5;

	std::map<int, Connection>::const_iterator it;
	for (it = _connections.begin(); it != _connections.end(); ++it) {

		long rem = it->second.secondsToClosestTimeout();
		if (rem < min_sec) {
			min_sec = static_cast<int>(rem);
		}
	}
	return (min_sec);
}
