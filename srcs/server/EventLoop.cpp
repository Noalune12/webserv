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

// delete or add as EventLoop member function
void	printWithoutR(std::string what, std::string line);

EventLoop::EventLoop(ServerManager& serverManager) : _epollFd(-1), _running(false), _serverManager(serverManager), _connections() {}

EventLoop::~EventLoop() {

	std::map<int, Connection>::iterator it;

	for (it = _connections.begin(); it != _connections.end(); ++it) {
		close(it->first);
	}
	_connections.clear();

	// cgi fds cleanup ?

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
	std::ostringstream oss;
	oss << "eventLoop initialized with " << listenFds.size() << " listen socket(s)";
	Logger::debug(oss.str());
	return (true);
}

void	EventLoop::run(void) {

	_running = true;
	struct epoll_event events[MAX_EVENTS];

	Logger::notice("eventLoop running...");

	while (_running) {
		int	timeout_sec = calculateEpollTimeout();
		int	nEvents = epoll_wait(_epollFd, events, MAX_EVENTS, timeout_sec * 1000);
		if (nEvents < 0) {
			if (errno == EINTR) // errno error for signal interruption
				continue ;
			Logger::error(std::string("epoll_wait() failed: ") + strerror(errno));
			break ;
		}

		checkTimeouts();

		for (int i = 0; i < nEvents; ++i) {

			int			fd = events[i].data.fd;
			uint32_t	ev = events[i].events;

			if (_serverManager.isListenSocket(fd))
				acceptConnection(fd);
			else if (_pipeToClient.find(fd) != _pipeToClient.end())
				handleCGIPipeEvent(fd, ev);
			else {
				handleClientEvent(fd, ev);
			}
		}
	}
	Logger::debug("eventLoop stopped"); // will have to be deleted since we get there if the server stops, and the only way to stop it is to send a SIGINT signal to the server. It gets printed after the signalHandling messages
}


void	EventLoop::handleCGIPipeEvent(int pipeFd, uint32_t ev) {

	// finds a client associated to a pipe
	std::map<int, int>::iterator it = _pipeToClient.find(pipeFd);
	if (it == _pipeToClient.end())
		return ;

	int	clientFd = it->second;
	std::map<int, Connection>::iterator clientIt = _connections.find(clientFd);
	if (clientIt == _connections.end()) {
		removeFromEpoll(pipeFd);
		close(pipeFd);
		_pipeToClient.erase(it);
		return ;
	}

	Connection& client = clientIt->second;

	if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) { // might not need all of those
		// if we get there that means the pipe is closed or there has been an error
		cleanupCGI(clientFd);
		client.setState(SENDING_RESPONSE); // need to send the data we have for this client
		client.startTimer(4, CLIENT_TIMEOUT);
		modifyEpoll(clientFd, EPOLLOUT);
		return ;
	}

	if (ev & EPOLLIN) {

		char	buffer[8192];
		ssize_t	bytes = read(pipeFd, buffer, sizeof(buffer));

		if (bytes > 0) {
			client._cgi.outputBuff.append(buffer, bytes);
			client.startTimer(3, CGI_TIMEOUT);
		} else if (bytes == 0) {
			cleanupCGI(clientFd);
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, CLIENT_TIMEOUT);
			modifyEpoll(clientFd, EPOLLOUT);
		} else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) { // checking EAGAIN and EWOULDBLOCK for now BUT NEEDS TO BE REMOVED!!!!
			cleanupCGI(clientFd);
			client._request.err = true;
			client._request.status = 502;
			client.setState(SENDING_RESPONSE);
			modifyEpoll(clientFd, EPOLLOUT);
		}
	}
}

bool	EventLoop::startCGI(int clientFd) {

	std::map<int, Connection>::iterator it = _connections.find(clientFd);
	if (it == _connections.end())
		return (false);

	Connection& client = it->second;
	CGIContext& cgi = client._cgi;

	if (pipe(cgi.pipeIn) == -1) {
		Logger::error("pipe(pipeIn) failed");
		return (false);
	}

	if (pipe(cgi.pipeOut) == -1) {
		close(cgi.pipeIn[0]);
		close(cgi.pipeIn[1]);
		Logger::error("pipe(pipeOut) failed");
		return (false);
	}

	cgi.pid = fork();

	if (cgi.pid == -1) {
		cgi.closePipes();
		Logger::error("fork() failed");
		return (false);
	}
	// implement children logic
	if (cgi.pid == 0) {
		std::cout << RED "IN CHILD PROCESS" RESET << std::endl;
		// closing child unused fds first
		close(cgi.pipeIn[1]);
		close(cgi.pipeOut[0]);

		// redir
		dup2(cgi.pipeIn[0], STDIN_FILENO);
		dup2(cgi.pipeOut[1], STDOUT_FILENO);

		// closing original fds
		close(cgi.pipeIn[0]);
		close(cgi.pipeOut[1]);

		std::map<int, Connection>::iterator it;
		for (it = _connections.begin(); it != _connections.end(); ++it) {
			close(it->first);
		}
		if (_epollFd >= 0) {
			close(_epollFd);
			_epollFd = -1;
		}

		std::vector<int>			serverSockets = _serverManager.getListenSocketFds();
		std::vector<int>::iterator	itServerSockets;

		for (itServerSockets = serverSockets.begin(); itServerSockets != serverSockets.end(); ++itServerSockets) {
			close(*itServerSockets);
		}





		// GROS DU BOULOT:
		// creation variable d'environnements
		// chdir vers l'endroit ou est le script
		// execve du script
		char* envp[] = {
			(char*)"SERVER_SOFTWARE=Webserv/1.0",
			(char*)"GATEWAY_INTERFACE=CGI/1.1",
			(char*)"SERVER_PROTOCOL=HTTP/1.1",
			(char*)"REQUEST_METHOD=GET",
			(char*)"SCRIPT_NAME=/cgi-test/hello.py",
			(char*)"SCRIPT_FILENAME=./var/www/cgi-test/hello.py",
			(char*)"PATH_INFO=/cgi-test",
			(char*)"QUERY_STRING=",  // à remplir si query string
			(char*)"REMOTE_ADDR=127.0.0.1",
			(char*)"CONTENT_LENGTH=0",
			NULL
		};

		char *argv[] = {(char*)"/usr/bin/python3", (char*)"./var/www/cgi-test/hello.py", NULL};
		execve("/usr/bin/python3", argv, envp);
		std::cerr << RED "execve failed: " << strerror(errno) << std::endl << RESET;
		_exit(1);
		// on est jamais sensé arrivé la, on verra comment je sors plus tard
		// std::exit(666);
		// _exit(666); -> le mieux mais est-ce qu'on a le droit ????
	}
	// implement parent logic

	// closing parent unused fds
	close(cgi.pipeIn[0]);
	close(cgi.pipeOut[1]);

	cgi.pipeIn[0] = -1;
	cgi.pipeOut[1] = -1;

	fcntl(cgi.pipeOut[0], F_SETFL, O_NONBLOCK); // a securisé

	if (!addToEpoll(cgi.pipeOut[0], EPOLLIN)) {
		cleanupCGI(clientFd);
		return (false);
	}

	_pipeToClient[cgi.pipeOut[0]] = clientFd;

	// POST -> need to write the body into the pipe

	// send EOF to the CGI
	Logger::debug("Sending EOF to CGI via pipeIn[1]");
	close(cgi.pipeIn[1]);
	cgi.pipeIn[1] = -1;

	client.setState(CGI_RUNNING);
	client.startTimer(3, CGI_TIMEOUT);

	return (true);
}

void	EventLoop::cleanupCGI(int clientFd) {

	std::map<int, Connection>::iterator it = _connections.find(clientFd);
	if (it == _connections.end())
		return ;

	CGIContext& cgi = it->second._cgi;

	if (cgi.pipeOut[0] != -1) {
		removeFromEpoll(cgi.pipeOut[0]);
		_pipeToClient.erase(cgi.pipeOut[0]);
	}

	cgi.closePipes();

	if (cgi.pid > 0) {
		int status;
		waitpid(cgi.pid, &status, WNOHANG); // the flag makes it non blocking (for the eventloop)
		cgi.pid = -1;
	}
}

void	EventLoop::handleClientEvent(int clientFd, uint32_t ev) {

	std::map<int, Connection>::iterator it = _connections.find(clientFd);

	if (it == _connections.end())
		return ;

	Connection& client = _connections[clientFd];

	if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		if (ev & EPOLLERR) {
			std::cerr << RED "EPOLLERR - fd[" << clientFd << "]" RESET << std::endl;
		} else if (ev & EPOLLHUP) {
			std::cerr << RED "EPOLLHUP - fd[" << clientFd << "]" RESET << std::endl;
		} else if (ev & EPOLLRDHUP) {
			std::cerr << RED "EPOLLRDHUP - fd[" << clientFd << "]" RESET << std::endl;
		}
		closeConnection(clientFd);
		return;
	}

	int timer_idx = getActiveTimer(client.getState());
	if (timer_idx >= 0 && client.isTimedOut(timer_idx)) {
		// sendTimeoutResponse(clientFd, client.getState());  // 408/504
		closeConnection(clientFd);
		return ;
	}

	switch (client.getState()) {

		case IDLE:
			client.setState(READING_HEADERS);
			client.startTimer(1, CLIENT_TIMEOUT);
			modifyEpoll(clientFd, EPOLLIN);
			Logger::debug("IDLE state");
			break ;

		case READING_HEADERS: // fallthrought ? not sure yet
			if (ev & EPOLLIN) {
				tempCall(clientFd);
				client.setState(SENDING_RESPONSE); // will need to set READING_BODY first
				client.startTimer(3, CLIENT_TIMEOUT);
				modifyEpoll(clientFd, EPOLLOUT);   // needs to be in the: case READING_BODY, not here
				printWithoutR("Request", client.getBuffer());
			}
			if (client.getBuffer().empty()) { // to avoid EPOLLERR
				closeConnection(clientFd);
				break ;
			}
			client.parseRequest();

			Logger::debug("CGI check for URI: " + client._request._uri);
			Logger::debug("cgiPath = '" + client._request._reqLocation.cgiPath + "'");
			Logger::debug("cgiExt  = '" + client._request._reqLocation.cgiExt + "'");

			if (!client._request._reqLocation.cgiExt.empty() || !client._request._reqLocation.cgiPath.empty()) {
				client._request._reqLocation.cgiPath = "var/www/cgi-test/hello.py";

				if (startCGI(clientFd)) {
					return ;
				} else {
					client._request.err = true;
					client._request.status = 500;
				}
			}

			if (client._request.chunkRemaining == true) {
				client.setState(READING_BODY);
				client.startTimer(2, CLIENT_TIMEOUT - 2);
				modifyEpoll(clientFd, EPOLLIN);
			} else if (client._request.err == false) { //not sure if it is here
				client._request.methodHandler();
			}

			Logger::debug("READING_HEADERS state");
			break ; // to remove if we fallthrought

		case READING_BODY:
			if (ev & EPOLLIN) {
				size_t n = tempCall(clientFd);
				if (n == 0) {
					std::cout << "NOTHING RECEIVED" << std::endl;
					closeConnection(clientFd);
					break ;
				}
				client._request._chunk += client.getBuffer();
				client.startTimer(2, CLIENT_TIMEOUT - 2);
				// printWithoutR("Body", client.getBuffer());
			}
			client._request.parseChunk();
			if (client._request.chunkRemaining == false || client._request.err == true) {
				client.setState(SENDING_RESPONSE);
				client.startTimer(4, CLIENT_TIMEOUT);
				modifyEpoll(clientFd, EPOLLOUT);
			}

			Logger::debug("READING_BODY state");
			break ;

		case CGI_RUNNING:
			// si on arrive ici c'est probablement qu'il y a eu une erreur (les CGI sont handle dans handleCGIPipeEvent)
			// On ne check que pour les erreurs et on laisse tourner
			if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) { // je laisse pour identifier quelles erreurs on chope ici de la meme facon que plus haut
				if (ev & EPOLLERR) {
					std::cerr << RED "EPOLLERR - fd[" << clientFd << "]" RESET << std::endl;
				} else if (ev & EPOLLHUP) {
					std::cerr << RED "EPOLLHUP - fd[" << clientFd << "]" RESET << std::endl;
				} else if (ev & EPOLLRDHUP) {
					std::cerr << RED "EPOLLRDHUP - fd[" << clientFd << "]" RESET << std::endl;
				}
				if (client._cgi.pid > 0) {
					kill(client._cgi.pid, SIGKILL);
				}
				Logger::debug("Client disconnection while CGI running");
				cleanupCGI(clientFd);
				closeConnection(clientFd);
			}
			break ;

		case SENDING_RESPONSE:
		std::cout << "BODY = " << client._request._body << std::endl;
			if (ev & EPOLLOUT) {

				Response response;
				response.debugPrintRequestData(client._request);

				response.prepare(client._request);

				std::vector<char> buffer = response.buildRaw();

				ssize_t sent = send(clientFd, &buffer[0], buffer.size(), 0);

				if (sent > 0) {
					std::cout << GREEN "[fd " << clientFd << "] Sent " << sent
							<< " bytes, status=" << response._statusCode << RESET << std::endl;
				} else if (sent < 0) {
					std::cerr << RED "[fd " << clientFd << "] send() error: "
							<< strerror(errno) << RESET << std::endl;
				}

				if (client._request._keepAlive && !client._request.err) {
					client.setState(IDLE);
					client.startTimer(0, CLIENT_TIMEOUT);
					modifyEpoll(clientFd, EPOLLIN);
				} else {
					closeConnection(clientFd);
				}
			}
			Logger::accessLog(client.getIP(), "method", "uri", "version", 666, 100); // temp, won't we called here
			Logger::debug("SENDING_RESPONSE state");
			if (client._request._keepAlive == false)
				closeConnection(clientFd);
			break ;

		case CLOSED: // not sure we need it tbh since we keep alive the connection, and if the socket timeouts its identified somewhere else
			closeConnection(clientFd);
			break ;
	}
}

size_t	EventLoop::tempCall(int clientFd) {
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
				return bytes;
			}
			buffer += std::string(buf, bytes);
		}
		// std::string req = buffer;
		// std::cout << YELLOW "Message from fd[" << clientFd << "]:\n" RESET << buffer;
		std::map<int, Connection>::iterator it = _connections.find(clientFd);
		it->second.setBuffer(buffer);
		return bytes;
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
	Connection newClient(clientFd, clientIp, clientPort, _serverManager.getServers(), _serverManager.getGlobalDir());

	newClient.setState(IDLE);
	newClient.startTimer(IDLE, CLIENT_TIMEOUT);

	if (!addToEpoll(clientFd, EPOLLIN)) { // not sure if I HAVE to add EPOLLIN and EPOLLOUT here
		close(clientFd);
		return ;
	}

	_connections[clientFd] = newClient;

	std::ostringstream	oss;
	oss << "new client #" << clientFd << " from " << clientIp << ":" << clientPort;
	Logger::notice(oss.str());
}




/* utils */
// bool	EventLoop::setNonBlocking(int fd) {
// 	return (fcntl(fd, F_SETFL, O_NONBLOCK) >= 0);
// }


void EventLoop::sendError(int clientFd, int status) {
	std::cout << GREEN "Sending " << status << " response to fd[" << clientFd << "]" RESET << std::endl;
	Connection& client = _connections[clientFd];

	std::string statusName;
	if (status == 400)
		statusName = "Bad Request";
	if (status == 404)
		statusName = "Not found";
	if (status == 405)
		statusName = "Method Not Allowed";
	if (status == 403)
		statusName = "Forbidden";
	if (status == 505)
		statusName = "HTTP Version Not Supported";

	std::stringstream ss;
    ss << status;
	std::string statusReturn = ss.str();
	std::string body;
	if (client._request.htmlPage.empty()) {
		body =
			"<html>\n"
			"<head><title>" + statusReturn + " " + statusName + "</title></head>\n"
			"<body><h1>" + statusReturn + " " + statusName + "</h1>\n"
			"</body>\n"
			"</html>\n";
	} else {
		body = client._request.htmlPage;
	}

    std::stringstream sss;
    sss << body.size();
    std::string bodySize = sss.str();

    std::string response =
        "HTTP/1.1 " + statusReturn + " " + statusName + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + bodySize + "\r\n"
        "\r\n" +
        body;

	send(clientFd, response.c_str(), response.size(), 0); // flags no use ? MSG_NOSIGNAL | MSG_DONTWAIT | also MSG_OOB

	// if (client._request._keepAlive == false)
	// 	closeConnection(clientFd);
	Logger::accessLog(client.getIP(), "method", "uri", "version", -1, body.size());
	// std::cout << GREEN "Sent " << sent << " bytes to fd[" << clientFd << "]" RESET << std::endl;
}


// delete if not used
void EventLoop::sendStatus(int clientFd, int status) {
	std::cout << GREEN "Sending " << status << " response to fd[" << clientFd << "]" RESET << std::endl;
    Connection& client = _connections[clientFd];

	std::stringstream ss;
    ss << status;
	std::string statusReturn = ss.str();

	std::string body;
	// if (client.htmlPage.empty()) {
	// 	body =
	// 		"<html>\n"
	// 		"<head><title>" + statusReturn + "</title></head>\n"
	// 		"</body>\n"
	// 		"</html>\n";
	// } else {
		body = client._request.htmlPage;
	// }

    std::stringstream sss;
    sss << body.size();
    std::string bodySize = sss.str();

	std::string res;
	if (status == 200)
		res = "OK";

    std::string response =
        "HTTP/1.1 " + statusReturn + " " + res + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + bodySize + "\r\n"
        "\r\n" +
        body;

	send(clientFd, response.c_str(), response.size(), 0); // flags no use ? MSG_NOSIGNAL | MSG_DONTWAIT | also MSG_OOB

	// Connection& client = _connections[clientFd];
	Logger::accessLog(client.getIP(), "method", "uri", "version", -1, body.size());
	// if (client._request._keepAlive == false)
	// 	closeConnection(clientFd);
	// std::cout << GREEN "Sent " << sent << " bytes to fd[" << clientFd << "]" RESET << std::endl;
}
