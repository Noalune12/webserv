#include <iostream>
#include <fstream>
#include <sstream>

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
	// J'imagine qu'on peu stocker le nom du fichier si il est valide dans this->_filePath plutot que de retourner un booléen
	// ca permettra a la fonction ci-dessous d'open directement le ifstream via ses membres privées
	return (file_extension == extension);
}

std::string	FileReader::readFile(void) {

	std::ifstream fileName("default.conf"); // temporaire, on le récupérera autrement ?
	// fileName.close(); // test 1 pour que ca pète

	if (!fileName.is_open()) {
		std::cerr << "Error: opening file stream" << std::endl;
	}

	std::stringstream buffer;

	// fileName.close(); // test 2 pour que ca pète
	buffer << fileName.rdbuf();
	if (!fileName.fail()) { // il me semble que c'est plus juste de checker !buffer car ca check le retour de .rdbuf mais aussi l'insertion dans buffer
		std::cerr << "Error: extraction or insertion failed" << std::endl;
	}

	std::string res = buffer.str();

	return (res);
}


FileReader::~FileReader() {}
