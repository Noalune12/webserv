#ifndef EVENTLOOP_HPP
# define EVENTLOOP_HPP

# include <stdint.h>

# include "Connection.hpp"
# include "ServerManager.hpp"

class EventLoop {

	private:
		static const int	MAX_EVENTS = 1024;	// to be define

		int					_epollFd;		// epoll instance
		bool				_running;		// Main loop control
		ServerManager&		_serverManager;	// Manages listen sockets and vhosts

		// client socket fd, Connection object
		std::map<int, Connection>	_connections;

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

		void	handleClientTest(int clientFd, uint32_t ev);

		void	send400(int clientFd);

		void	tempCall(int clientFd);
	};

#endif
