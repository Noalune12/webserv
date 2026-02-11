#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

/**
 *
 * Exercice 1 : Création d'un socket
 * Objectif : Écrire un programme minimal qui crée un socket TCP IPv4 et vérifie si la création a réussi.
 * Consignes :
 *
 * Créer un socket avec socket()
 * Vérifier si la création a réussi (retour != -1)
 * Afficher le file descriptor du socket si succès, ou un message d'erreur sinon
 * Fermer proprement le socket avant de quitter
 *
 * Questions de compréhension (réponds-y dans ton code en commentaires) :
 *
 * Que signifient les trois paramètres de socket() ?
 * Pourquoi utilise-t-on AF_INET et SOCK_STREAM pour un serveur HTTP ?
 */

int	main(void)
{
	// int				status;
	// struct addrinfo	hints;
	// struct addrinfo	*res;
	// status = getaddrinfo("localhost", "http", &hints, &res); // didn't check the return of getaddrinfo but I believe we can use gai_strerror()
	// socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	int	socket_fd;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	// Que signifient les trois paramètres de socket() ?
	// Le premier parametre de socket est le domain de communication, ai_family ou AF_INET dans notre cas
	// Le 2e parametre est le type de socket utiliser ai_socktype ou SOCK_STREAM pour un stream qui se sert du protocole TCP pour communiquer
	// Le 3e est un protocole particulier, dans notre cas 0 est suffisant.
	// basicaly the same as: socket(AF_INET, SOCK_STREAM, 0) ?
	// Pourquoi utilise-t-on AF_INET et SOCK_STREAM pour un serveur HTTP ?
	// On utilise SOCK_STREAM car il se sert du protocole TCP pour communiquer.
	// On utiliset AF_INET car il represente l'IPv4 Internet protocole

	if (socket_fd == -1) {
		std::cerr << "error creating socket" << std::endl;
		return (EXIT_FAILURE);
	}

	std::cout << "socket_fd: " << socket_fd << std::endl;

	// a socket is a fd so we can close it easily
	close(socket_fd);

	return (EXIT_SUCCESS);
}
