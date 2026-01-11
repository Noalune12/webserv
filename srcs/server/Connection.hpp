#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <ctime>
# include <iostream>
# include <sys/types.h>

enum ConnectionState {
	READING_REQUEST, // starting state ? keeping this state until chunked requests are completely read
	PROCESSING,
	CLOSING
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
		std::string			_buffer;
		ssize_t				_bufferLenght; // or is it _requestLenght ? -> might be able to help you identify chunked mode
		bool				_keepAlive;
		// bool				_chunked;
		time_t				_lastActivity;

	public:
		Connection(); // cannot compile without it and I don't understand why...
		Connection(int& clientFd, std::string& ip, int& port);
		~Connection();

		void	updateLastActivity(void);
		bool	isTimedOut(time_t timeout) const;

		/* getters */
		const std::string&	getIP(void) const;
		time_t				getLastActivity(void) const;

};

#endif
