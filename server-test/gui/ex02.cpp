#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

/**
 * Exercice 2 : Configurer l'adresse et faire un bind()
 * Objectif : Configurer une structure sockaddr_in et lier le socket à une adresse/port avec bind().
 * Consignes :
 *
 * Créer un socket (comme exercice 1)
 * Déclarer et initialiser une struct sockaddr_in avec :
 *
 * Famille : IPv4
 * Port : 8080 (attention à l'ordre des octets !)
 * Adresse : toutes les interfaces (0.0.0.0)
 *
 *
 * Lier le socket à cette adresse avec bind()
 * Afficher un message de succès ou d'erreur
 * Fermer proprement
 *
 * Questions de compréhension :
 *
 * À quoi sert htons() ? Pourquoi est-ce nécessaire ?
 * Que signifie INADDR_ANY ?
 * Pourquoi doit-on caster sockaddr_in* en sockaddr* dans bind() ?
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
	address.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY veut dire qu'on "bind to all interfaces"
	address.sin_port = htons(8080); // htons permet la convertion de l'addresse dans l'ordre des octets du reseau, host to network short

	// On cast sockaddr_in * en sockaddr* car c'est le type de donnee que prend bind en parametre ?
	if (bind(socket_fd, ((struct sockaddr*) &address), sizeof(address)) == -1) {
		std::cout << "binding failed" << std::endl;
		return (1);
	}

	std::cout << "binding succeed" << std::endl;
	close(socket_fd);
	return (0);
}


// htons() : Correct ! Précision : le réseau utilise toujours big-endian, mais ta machine est probablement little-endian — htons fait la conversion si nécessaire.
// Cast sockaddr* : Oui, et la raison historique est que bind() est une fonction générique qui fonctionne avec plusieurs familles d'adresses (IPv4, IPv6, Unix sockets...). sockaddr est la structure "générique", sockaddr_in est la spécialisation pour IPv4.
