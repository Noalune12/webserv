#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <ctime>
# include <iostream>
# include <sys/types.h>
# include <unistd.h>
# include <vector>

# include "ConfigInheritor.hpp"
# include "Request.hpp"
# include "Response.hpp"

enum ConnectionState {
	IDLE,				// 0. starting state
	READING_HEADERS,	// 1. keeping this state until chunked requests are completely read
	READING_BODY,		// 2. nd state
	CGI_RUNNING,		// 3. only if CGI
	SENDING_RESPONSE,	// 4.
};

struct CGIContext {
	pid_t		pid;
	int			pipeIn[2];	// stdin (server puis CGI)
	int			pipeOut[2];	// stdout (CGI puis server)
	std::string	outputBuff;
	bool		headerParsed;
	size_t		bodyStart;

	CGIContext() : pid(-1), headerParsed(false), bodyStart(0) {
		pipeIn[0] = -1;
		pipeIn[1] = -1;
		pipeOut[0] = -1;
		pipeOut[1] = -1;
	}

	void	closePipes(void) {
		if (pipeIn[0] != -1) {
			close(pipeIn[0]);
			pipeIn[0] = -1;
		}
		if (pipeIn[1] != -1) {
			close(pipeIn[1]);
			pipeIn[1] = -1;
		}
		if (pipeOut[0] != -1) {
			close(pipeOut[0]);
			pipeOut[0] = -1;
		}
		if (pipeOut[1] != -1) {
			close(pipeOut[1]);
			pipeOut[1] = -1;
		}
	}

	bool	isActive() {
		return (pid > 0);
	}
};

// On pourrait la renommer Client la classe
// Avec l'architecture que j'ai en tete, c'est ici qu'on stock absolument toutes les donnees de chaque clients
// Informations, etats, buffers, truc pour les CGI, tout!

// note: pour les fd des CGI (pipes), ne pas oublier de les mettre dans la interest list d'epoll

class Connection {

	private:
		int					_clientFd;
		std::string			_ip;
		int					_port;
		ConnectionState		_state;
		time_t				_timers[5];
		std::string			_buffer;
		ssize_t				_bufferLenght; // or is it _requestLenght ? -> might be able to help you identify chunked mode
		bool				_keepAlive;
		// bool				_chunked;

		std::vector<server>	_servers;

		std::string _serverIP;
		int			_serverPort;

		public:
		Connection(); // cannot compile without it and I don't understand why...
		Connection(int& clientFd, std::string& ip, int& port, std::vector<server>	servers, globalDir globalDir);
		~Connection();

		/* timeout related functions */
		void	startTimer(int index, time_t duration);
		bool	isTimedOut(int index) const;
		long	secondsToClosestTimeout(void) const;

		/* getters */
		const std::string&	getIP(void) const;
		ConnectionState		getState(void) const;
		std::string			getBuffer(void) const;

		/* setters */
		void				setState(ConnectionState s);
		void				setBuffer(std::string request);

		void parseRequest();
		// bool				err;
		// int					status;
		Request				_request;
		// std::string			htmlPage;
		CGIContext			_cgi;
};

#endif
