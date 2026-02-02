#ifndef EVENTLOOP_HPP
# define EVENTLOOP_HPP

# include <stdint.h>

# include "Connection.hpp"
# include "ServerManager.hpp"

# define PROXY_AUTH_REQ 407

class EventLoop {

	private:
		static const int	MAX_EVENTS = 1024;		// to be define
		static const time_t	CLIENT_TIMEOUT = 5;	// 5s, idk what is a good timeout value
		static const time_t	CGI_TIMEOUT = 3;


		int					_epollFd;		// epoll instance
		bool				_running;		// Main loop control
		ServerManager&		_serverManager;	// Manages listen sockets and vhosts

		// client socket fd, Connection object
		std::map<int, Connection>	_connections;
		// pipeFd, clientFd
		std::map<int, int>			_pipeToClient;

	public:
		EventLoop(ServerManager& serverManager);
		~EventLoop();

		bool	init(void);

		void	run(void);
		void	stop(void);

		bool	addToEpoll(int fd, uint32_t events);
		bool	modifyEpoll(int fd, uint32_t events);
		bool	removeFromEpoll(int fd);

		void	closeConnection(int clientFd);
		// bool	setNonBlocking(int fd);

		bool	isRunning(void) const;
		size_t	getConnectionCount(void) const;

		void	acceptConnection(int listenFd);

		void	getClientInfo(struct sockaddr_in& addr, std::string& ip, int& port);

		void	handleClientEvent(int clientFd, uint32_t ev);

		// cgi
		void	handleCGIPipeEvent(int pipeFd, uint32_t ev);
		bool	startCGI(int clientFd);
		void	cleanupCGI(int clientFd);

		void	sendError(int clientFd, int status);
		void	sendStatus(int clientFd, int status);

		size_t	tempCall(int clientFd);

		void	checkTimeouts(void);
		int		calculateEpollTimeout(void);
		int		getActiveTimer(ConnectionState s);
};

#endif
