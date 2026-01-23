#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>


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
		std::cerr << "binding failed: ";
		dprintf(2, "%s\n", strerror(errno));
		close(socket_fd);
		return (1);
	}

	if (listen(socket_fd, 10) != -1) {
		std::cout << "Server listening on port 8080" << std::endl;
	}
	else {
		close(socket_fd);
	}
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	addr_size = sizeof(client_addr);

	int client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &addr_size);
	if (client_fd != -1) {
		std::cout << "Client connected!\nClient fd: " << client_fd << std::endl;
	}

	char buffer[1024];

	// recv retourne le nombre de bytes recu, un retour de 0 veut dire qu'il a fini de recevoir les informations de la socket et qu'elle s'est fermee ? et/ou a recu EOF
	// OUTDATED and quite false
	// if (recv(client_fd, buffer, 1024, 0) == -1) {
	// 	std::cerr << "recv() error: ";
	// 	dprintf(2, "%s\n", strerror(errno));
	// 	close(socket_fd);
	// 	close(client_fd);
	// 	return (1);
	// }
	// dprintf(2, "buffer content: %s\n", buffer);

	/* correction */
	ssize_t bytes_received = recv(client_fd, buffer, 1023, 0);  // 1023 pour garder place pour \0
	if (bytes_received > 0) {
		buffer[bytes_received] = '\0';  // Null-terminer
		printf("buffer content: %s\n", buffer);
	}

	if (send(client_fd, "Hello from server!\n", 20, 0) == -1) {
		std::cerr << "Error while sending message from client" << std::endl;
		close(socket_fd);
		close(client_fd);
		return (1);
	}

	std::cout << "Closing server" << std::endl;
	if (socket_fd)
		close(socket_fd);
	if (client_fd)
		close(client_fd);

	return (0);
}

// On utilise recv et send plutot que read et write car les paquets de bytes peuvent arriver par chunk, et on ne voudrait pas arreter de lire avant d'avoir recu/envoyer la totalité du message.
