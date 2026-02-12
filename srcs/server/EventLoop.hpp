#ifndef EVENTLOOP_HPP
# define EVENTLOOP_HPP

# include "CGIExecutor.hpp"
# include "Connection.hpp"
# include "ResponseBuilder.hpp"
# include "ServerManager.hpp"

# define PROXY_AUTH_REQ 407

class EventLoop {

	private:
		static const int    MAX_EVENTS = 1024;	// to be defined
		static const time_t CLIENT_TIMEOUT = 5; // to be defined
		static const time_t CGI_TIMEOUT = 5;	// to be defined

		int                         _epollFd;		// epoll instance
		bool                        _running;		// Main loop control
		ServerManager&              _serverManager;	// Manages listen sockets and vhosts
		std::map<int, Connection>   _connections;	// client socket fd, Connection object
		std::map<int, int>          _pipeToClient;	// pipeFd, clientFd

		CGIExecutor     _cgiExecutor;
		ResponseBuilder _responseBuilder;

	public:
		EventLoop(ServerManager& serverManager);
		~EventLoop();

		bool	init(void);
		void	run(void);
		void	stop(void);

		bool	addToEpoll(int fd, uint32_t events);
		bool	modifyEpoll(int fd, uint32_t events);
		bool	removeFromEpoll(int fd);

		void	acceptConnection(int listenFd);
		void	closeConnection(int clientFd);

		void	handleClientEvent(int clientFd, uint32_t ev);
		void	handleCGIPipeEvent(int pipeFd, uint32_t ev);

		void	checkTimeouts(void);
		int		calculateEpollTimeout(void);
		int		getActiveTimer(ConnectionState s);

		bool				isRunning(void) const;
		size_t				getConnectionCount(void) const;
		std::vector<int>	getAllClientFds(void) const;
		std::vector<int>	getListenSocketFds(void) const;
		void				closeEpollFd(void);

		void	registerPipe(int pipeFd, int clientFd);
		void	unregisterPipe(int pipeFd);

		void	handleIdle(Connection& client, int clientFd, uint32_t ev);
		void	handleReadingHeaders(Connection& client, int clientFd, uint32_t ev);
		void	handleReadingBody(Connection& client, int clientFd, uint32_t ev);
		void	handleCGIRunning(Connection& client, int clientFd, uint32_t ev);
		void	handleSendingResponse(Connection& client, int clientFd, uint32_t ev);

		void	transitionToIDLE(Connection& client, int clientFd);
		void	transitionToReadingBody(Connection& client, int clientFd);
		void	transitionToCGI(Connection& client, int clientFd);
		void	transitionToSendingResponse(Connection& client, int clientFd);

		size_t	readFromClient(int clientFd, Connection& client);

		bool	checkEpollErrors(uint32_t ev, int clientFd);
		bool	checkTimeout(Connection& client, int clientFd);

		void	getClientInfo(struct sockaddr_in& addr, std::string& ip, int& port);
};


#endif
