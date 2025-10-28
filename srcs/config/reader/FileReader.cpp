#include <iostream>
#include <fstream>

#include "FileReader.hpp"


FileReader::FileReader() {}

FileReader::FileReader(const std::string& filePath) : _filePath(filePath) {}

bool	FileReader::extensionVerification(const std::string& configFile, const char *extension)
{
	std::ifstream fileStream(configFile.c_str());
	if (!fileStream) {
		std::cerr << "Error: cannot open file '" << configFile << "'" << std::endl;
		return (false);
	}

	size_t last_dot = configFile.find_last_of('.');
	size_t last_slash = configFile.find_last_of('/');

	// recupère la position exact du début du nom du fichier, apres les / si il y a des sous-dossiers
	size_t filename_start = (last_slash == std::string::npos) ? 0 : last_slash + 1;


	// dégage les fichiers sans extension et les .conf ou /.conf MAIS accepte .conf.conf
	if (last_dot == std::string::npos || last_dot == filename_start) {
		return (false);
	}

	std::string file_extension = configFile.substr(last_dot);

	// pour l'instant retourne un booléen mais peut etre qu'on peut changer ca pour des exceptions perso ?
	// Je sais pas ce qui a le plus d'intéret/meilleure pratique
	return (file_extension == extension);
}


FileReader::~FileReader() {}
