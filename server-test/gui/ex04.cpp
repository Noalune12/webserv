#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

/**
 * Exercice 4 : Accepter une connexion avec accept()
 * Objectif : Accepter une connexion entrante et récupérer un nouveau file descriptor pour communiquer avec le client.
 * Consignes :
 *
 * Reprendre le code de l'exercice 3
 * Remplacer le sleep(30) par un appel à accept()
 * Stocker le fd retourné par accept() dans une variable client_fd
 * Afficher "Client connected!" et le fd du client
 * Fermer client_fd ET socket_fd proprement
 *
 * Questions :
 *
 * Quelle est la différence entre socket_fd et client_fd ? Pourquoi deux fd ?
 * Que fait accept() quand aucun client n'est connecté ? (comportement bloquant)
 *
 * Test : Lance ton serveur, puis nc localhost 8080 dans un autre terminal. Le message "Client connected!" devrait s'afficher.
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

	// On a un 2e fd ouvert, qui doit etre celui du client, c'est aussi une socket. 2 fd car comme dans une discussion il faut 2 personnes pour communiquer, ici un client, un server
	int client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &addr_size);
	// Si aucun client n'est connecté, accept est bloquant jusqu'a ce qu'une connection soit presente. Par contre si la socket est marquer non bloquante, alors ce ne sera pas le cas et accept fail, et renvoie EAGAIN ou EWOULDBLOCK
	if (client_fd != -1) {
		std::cout << "Client connected!\nClient fd: " << client_fd << std::endl;
	}

	std::cout << "Closing server" << std::endl;
	close(socket_fd);
	close(client_fd);

	return (0);
}

// avec nc localhost 8080, on a bien un message de client connected cote server, ensuite les socket sont fermer et le server se ferme correctement, par contre on a besoin de faire entrer coter terminal pour actualisé la fermeture du stream
