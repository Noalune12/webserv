#ifndef CONNECTION_HPP
# define CONNECTION_HPP

enum ConnectionState {
	READING_REQUEST, // starting state ? keeping this state until chunked requests are completely read
	PROCESSING,
	CLOSING
};

// On pourrait la renommer Client la classe
// Avec l'architecture que j'ai en tete, c'est ici qu'on stock absolument toutes les donnees de chaque clients
// Informations, etats, buffers, truc pour les CGI, tout!
class Connection {

	private:
		int					_clientFd;
		ConnectionState		_state;

	public:
		Connection();
		~Connection();
};

#endif
