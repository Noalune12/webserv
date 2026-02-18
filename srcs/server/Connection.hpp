#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <unistd.h>

# include "Response.hpp"

enum ConnectionState {
	IDLE,
	READING_HEADERS,
	READING_BODY,
	CGI_WRITING_BODY,
	CGI_RUNNING,
	SENDING_RESPONSE
};

struct CGIContext {
	pid_t		pid;
	int			pipeIn[2];	// stdin (server then CGI)
	int			pipeOut[2];	// stdout (CGI then server)
	std::string	outputBuff;
	std::string	inputBody;
	size_t		inputOffset;

	CGIContext() : pid(-1) {
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

class Connection {

	private:
		std::string			_ip;
		ConnectionState		_state;
		time_t				_timers[5];
		std::string			_buffer;
		std::string			_chunkBuffer;
		std::vector<server>	_servers;
		std::string 		_serverIP;
		int					_serverPort;

	public:
		Connection(); // cannot compile without it
		Connection(int& clientFd, std::string& ip, std::vector<server>	servers, globalDir globalDir);
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
		void				clearBuffer();
		void				clearSendBuffer();
		void 				parseRequest();

		Request				request;
		CGIContext			cgi;
		std::vector<char>   sendBuffer;
		size_t              sendOffset;
		int					debug;
};

#endif
