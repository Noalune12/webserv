#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

/**
 * Exercice 5 : Recevoir et envoyer des données (recv/send)
 * Objectif : Lire ce que le client envoie et lui répondre.
 * Consignes :
 *
 * Après accept(), utiliser recv() pour lire les données du client dans un buffer (1024 octets)
 * Afficher ce que le client a envoyé
 * Envoyer une réponse au client avec send() : "Hello from server!\n"
 * Fermer proprement
 *
 * Questions :
 *
 * Que retourne recv() ? Que signifie un retour de 0 ?
 * Pourquoi utiliser recv()/send() plutôt que read()/write() sur des sockets ?
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

	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	addr_size = sizeof(client_addr);

	int client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &addr_size);
	if (client_fd != -1) {
		std::cout << "Client connected!\nClient fd: " << client_fd << std::endl;
	}

	std::cout << "Closing server" << std::endl;
	close(socket_fd);
	close(client_fd);

	return (0);
}

