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
	IDLE,
	READING_HEADERS,
	READING_BODY,
	CGI_WRITING_BODY,
	CGI_RUNNING,
	SENDING_RESPONSE,
};

struct CGIContext {
	pid_t		pid;
	int			pipeIn[2];	// stdin (server then CGI)
	int			pipeOut[2];	// stdout (CGI then server)
	std::string	outputBuff;
	bool		headerParsed;
	size_t		bodyStart;
	std::string	inputBody;
	size_t		inputOffset;

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
		std::string			_chunkBuffer;
		// bool				_chunked;

		std::vector<server>	_servers;

		std::string _serverIP;
		int			_serverPort;

	public:
		Connection(); // cannot compile without it
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
		std::string 		getChunkBuffer(void) const;

		/* setters */
		void				setState(ConnectionState s);
		void				setBuffer(std::string request);
		void				setChunkBuffer(std::string request);

		void				clearChunkBuffer();

		void parseRequest();
		// bool				err;
		// int					status;
		Request				_request;
		// std::string			htmlPage;
		CGIContext			_cgi;
};

#endif
