#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "FileReader.hpp"

#define EXTENSION ".conf"

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

	size_t filename_start = (last_slash == std::string::npos) ? 0 : last_slash + 1;

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

	if (!fileName.is_open()) {
		throw std::runtime_error("Cannot open file: " + _filePath);
	}

	std::stringstream buffer;

	buffer << fileName.rdbuf();
	if (!buffer) {
		fileName.close();
		throw std::runtime_error("Error retrieving configuration file: " + _filePath);
	}
	fileName.close();
	_fileContent = buffer.str();
}

const std::string&	FileReader::getFileContent(void) const {
	return (_fileContent);
}

const std::string&	FileReader::getFilePath(void) const {
	return (_filePath);
}

FileReader::~FileReader() {}
