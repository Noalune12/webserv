#include <iostream>
#include <fstream>
#include <sstream>

#include <stdexcept>


#include "FileReader.hpp"

// define car on sait qu'on veut accepter que ce format, pas besoin que ce soit en parametre des fonctions, cette valeur ne changera jamais
#define EXTENSION ".conf"

FileReader::FileReader() {}

FileReader::FileReader(const std::string& filePath) : _filePath(filePath) {

	try {
		extensionVerification(_filePath);
	} catch (const std::exception& e) {
		throw std::runtime_error("Invalid configuration file: " + std::string(e.what()));
	}

	try {
		readFile();
	} catch(const std::exception& e) {
		throw std::runtime_error("Failed to load configuration file: " + std::string(e.what()));
	}
}

void	FileReader::extensionVerification(const std::string& configFile)
{
	size_t last_dot = configFile.find_last_of('.');
	size_t last_slash = configFile.find_last_of('/');

	// recupère la position exact du début du nom du fichier, apres les / si il y a des sous-dossiers
	size_t filename_start = (last_slash == std::string::npos) ? 0 : last_slash + 1;

	// dégage les fichiers sans extension et les .conf ou /.conf MAIS accepte .conf.conf
	if (last_dot == std::string::npos || last_dot == filename_start) {
		throw std::invalid_argument("Missing file extension");
	}

	std::string file_extension = configFile.substr(last_dot);

	if (file_extension != EXTENSION) {
		throw std::invalid_argument("Invalid extension: expected " + std::string(EXTENSION) + ", got " + file_extension);
	}
}

void	FileReader::readFile(void) {

	std::ifstream fileName(_filePath.c_str());

	// fileName.close(); // test 1 pour que ca pète
	if (!fileName.is_open()) {
		throw std::runtime_error("Cannot open file: " + _filePath);
	}

	std::stringstream buffer;

	// fileName.close(); // test 2 pour que ca pète
	buffer << fileName.rdbuf();
	// fileName.clear(std::ios::badbit); // test 3 pour que ca pète
	// il me semble que c'est plus juste de checker !buffer car ca check le retour de .rdbuf (extraction) mais aussi l'insertion dans buffer
	// (autre check -> !fileName.fail())
	// !buffer ca check aussi si le fichier est vide, décision a prendre: message d'erreur global ou spécifique
	if (!buffer) {
		fileName.close();
		if (buffer.str().empty()) {
			throw std::runtime_error("Configuration file is empty: " + _filePath);
		}
		throw std::runtime_error("Error reading file: " + _filePath);
	}
	fileName.close();
	_fileContent = buffer.str();
}

const std::string&	FileReader::getFileContent(void) const {
	return (this->_fileContent);
}


FileReader::~FileReader() {}
