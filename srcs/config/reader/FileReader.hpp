#ifndef FILEREADER_HPP
# define FILEREADER_HPP

# include <string>

/*
 * Lecture du fichier de configuration
 *
 * Cette classe elle:
 * - Gère les erreurs d'ouverture/lecture (fichier inexistant, permissions, etc...)
 * - Ouvre et lit le fichier de configuration
 * - Retourne le contenu brut du fichier (comme ca on a un buffer ou y'a tout dedans et on peut commencer a le parse ?)
 */
class FileReader {

	private:
		std::string _filePath;

		FileReader();

		// Exception si le fichier n'existe pas ou n'est pas accessible ?
		bool	extensionVerification(const std::string& configFile, const char *extension);

	public:
		FileReader(const std::string& filePath);
		~FileReader();

		// Lit le fichier et retourne son contenu sous forme de string
		// std::string readFile();
};

#endif
