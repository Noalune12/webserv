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
 * Exercice 6 : Accepter plusieurs clients (boucle)
 * Objectif : Faire un serveur qui accepte les clients en boucle, un par un.
 * Consignes :
 *
 * Mettre le accept() → recv() → send() → close(client_fd) dans une boucle while(true)
 * Le serveur doit continuer à tourner après chaque client
 * Afficher le numéro du client (1, 2, 3...)
 *
 * Test : Connecte-toi plusieurs fois avec nc localhost 8080, le serveur doit rester actif.
 * Question : Quel est le problème de cette approche pour un vrai serveur web ?
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


    while (true)
    {
        static int client_count = 1;

        int client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &addr_size);
        if (client_fd != -1) {
            std::cout << "Client connected!\nClient fd: " << client_fd << std::endl;
        }

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

// Le probleme de cette approche et que le serveur est bloquant ? J'ai beau me connecter avec 2 clients different en meme temps, la boucle va gerer le premier client, annoncer qu'il est connecté puis afficher son message puis faire la meme chose avec les clients suivante
// Cependant si j'envoie un message avec le 2e, 3e client etc avant le 1er, alors ces messages (ainsi que les messages de connections) n'apparaitrons qu'apres que le 1er client ai terminé d'avoir été traité (ici ca implique d'avoir envoyer un message puis fermer la connection du client)
