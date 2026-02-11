#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

/**
 * Exercice 3 : Mettre le socket en écoute avec listen()
 * Objectif : Faire en sorte que le socket accepte des connexions entrantes.
 * Consignes :
 *
 * Reprendre le code corrigé de l'exercice 2
 * Après bind(), appeler listen() avec un backlog de 10
 * Afficher "Server listening on port 8080"
 * Pour l'instant, juste faire une pause avec sleep(30) avant de fermer (pour pouvoir tester)
 *
 * Question :
 *
 * Que représente le paramètre "backlog" de listen() ?
 *
 * Test : Pendant que ton programme tourne, ouvre un autre terminal et tape nc localhost 8080. Que se passe-t-il ?
 */

int main(void)
{
	int	socket_fd = 0;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		std::cerr << "error creating socket" << std::endl;
		return (1);
	}

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);

	if (bind(socket_fd, ((struct sockaddr*) &address), sizeof(address)) == -1) {
		std::cout << "binding failed" << std::endl;
		return (1);
	}

	if (listen(socket_fd, 10) != -1) {
		std::cout << "Server listening on port 8080" << std::endl;
	}
	// le parametre backlog de listen() represente le nombre de connections autorisées dans la file d'attente de cette socket.
	sleep(30);

	std::cout << "Closing server" << std::endl;
	close(socket_fd);
	return (0);
}

// nc localhost 8080 a probablement connecter le 2nd terminal a la socket, mon 2e terminal s'est mis en attente jusqu'a la fermeture de la socket
