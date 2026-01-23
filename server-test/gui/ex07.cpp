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
#include <fcntl.h>

/**
 * Exercice 7 : Rendre le socket non-bloquant
 * Objectif : Configurer le socket en mode non-bloquant avec fcntl().
 * Consignes :
 *
 * Après socket(), utiliser fcntl() pour ajouter le flag O_NONBLOCK
 * Essayer d'appeler accept() sans client connecté
 * Observer et gérer le retour d'erreur (EAGAIN ou EWOULDBLOCK)
 * Questions :
 *
 * Que se passe-t-il maintenant quand accept() est appelé sans client en attente ?
 * Pourquoi ce comportement seul ne suffit pas pour un vrai serveur ? (indice : busy-waiting)
 */

int main(void)
{
	int	socket_fd = 0;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		std::cerr << "error creating socket" << std::endl;
		return (1);
	}
	fcntl(socket_fd, F_SETFL, O_NONBLOCK);
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

	if (listen(socket_fd, 10) == -1) {
		close(socket_fd);
		return (0);
	}

	std::cout << "Server listening on port 8080" << std::endl;

	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	addr_size = sizeof(client_addr);

    while (true)
    {
        static int client_count = 1;

        int client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &addr_size);
        if (client_fd != -1) {
            std::cout << "Connected client fd [" << client_fd << "]" << std::endl;
        }
		else {
			std::cout << "Waiting for client to connect" << std::endl;
			sleep(1);
			continue ;
		}
		// std::cout << "accept: " << strerror(errno) << std::endl;
        char buffer[1024];

        ssize_t bytes_received = recv(client_fd, buffer, 1023, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Null-terminer
            printf("Client [%d] message: %s", client_count, buffer);
            ++client_count;
        }

        if (send(client_fd, "message received\n", 18, 0) == -1) {
            std::cerr << "Error while sending message from client" << std::endl;
            close(socket_fd);
            close(client_fd);
            return (1);
        }
        if (client_fd)
            close(client_fd);
    }

	std::cout << "Closing server" << std::endl;
	if (socket_fd)
		close(socket_fd);

	return (0);
}
