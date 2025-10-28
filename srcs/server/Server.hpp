#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>

# include "Config.hpp"

/*
 * Classe principale, on aura probablement un vecteur de serveur quelque part
 * Pour pouvoir gerer si il y a plusieurs bloc serveur dans le fichier de config
 */
class Server {

	private:
		// Facade principale: gère tout le processus de parsing et validation du fichier de configuration
		Config _config;

		Server();

	public:
		// Constructeur: délègue le parsing du fichier à la facade Config
		Server(const std::string& configFile);
		~Server();
};

#endif
