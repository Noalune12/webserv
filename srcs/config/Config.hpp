#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <string>
# include <map>
# include <vector>

# include "Context.hpp"

/*
 * Facade principale pour le parsing du fichier de configuration
 *
 * Role(s):
 * 1. Facade: orchestre LexerFacade, Validator et ConfigInheritor
 * 2. Data holder: stocke la configuration parsée et validée
 *
 * Le constructeur effectue tout le processus:
 * - FileReader vérifie l'existence et l'accès au fichier de config
 * - LexerFacade parse le fichier
 * - Validator valide les directives
 * - ConfigInheritor applique l'heritage (global -> server -> location)
 * - Stocke le resultat final dans ses attributs prives
 */
class Config {

	private:
		std::string _filePath; // FileReader.getFilePath()
		std::string _fileContent; // FileReader.getFileContent()

		// Donnees de configuration parsées et validées (au fur et a mesure)
		// std::vector<ServerBlock> _servers;

		std::map<std::string, std::vector<std::string> >	_globalDirectives;
		std::vector<Context>								_context;

		// Config();

	public:


		// Constructeur: orchestre tout le parsing et stocke la configuration
		Config(const std::string& configFileContent);
		~Config();
		bool isOnlyWSpace(std::string line) const;
		void addDirective(std::string line);
		void printMap() const;
		// void removeWSpaces();
};

#endif
