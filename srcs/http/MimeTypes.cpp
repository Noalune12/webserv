#include <algorithm>
#include <cctype>

#include <fstream>
#include <sstream>
#include <iostream>

#include "MimeTypes.hpp"

std::map<std::string, std::string>	MimeTypes::_types;
bool								MimeTypes::_initialized = false;
std::string							MimeTypes::_defaultPath = "mime.types"; // "./etc/webserv/mime.types"
std::string							MimeTypes::_lastMimeType;

MimeTypes::MimeTypes(void) {}

MimeTypes::~MimeTypes(void) {}

std::string	MimeTypes::getExtension(const std::string& filename) {

	if (filename.empty())
		return ("");

	size_t		lastSlash = filename.find_last_of('/');

	std::string	basename;
	if (lastSlash != std::string::npos) {
		basename = filename.substr(lastSlash + 1);
	} else {
		basename = filename;
	}

	size_t	dotPos = basename.find_last_of('.');

	if (dotPos == std::string::npos || dotPos == 0)
		return ("");

	return (basename.substr(dotPos));
}

std::string	MimeTypes::getType(const std::string& extensionOrFilename) {

	if (!_initialized) {
		if (!load(_defaultPath)) {
			return ("application/octet-stream");
		}
	}

	std::string	ext;

	if (!extensionOrFilename.empty() && extensionOrFilename[0] == '.') {
		ext = extensionOrFilename;
	}
	else if (extensionOrFilename.find('.') == std::string::npos) {
		ext = "." + extensionOrFilename;
	}
	else {
		ext = getExtension(extensionOrFilename);
	}

	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

	std::map<std::string, std::string>::const_iterator it = _types.find(ext);

	if (it != _types.end())
		return (it->second);

	return ("application/octet-stream");
}

void	MimeTypes::parseLine(const std::string& line) {

	if (line.empty())
		return ;

	size_t firstNonSpace = line.find_first_not_of(" \t");
	if (firstNonSpace == std::string::npos)
		return ;

	char firstChar = line[firstNonSpace];
	if (firstChar == '#' || firstChar == '{' || firstChar == '}')
		return ;

	std::istringstream	iss(line);
	std::string			firstToken;

	iss >> firstToken;
	if (firstToken.empty())
		return ;

	bool isContinuationLine = ((firstNonSpace > 0) && (firstToken.find('/') == std::string::npos));

	std::string mimeType;
	std::string ext;

	//
	if (isContinuationLine) {
		mimeType = _lastMimeType;
		ext = firstToken;
	}
	else {
		mimeType = firstToken;
		_lastMimeType = mimeType;
		iss >> ext;
	}

	// less strict than config file, only checking if the semicolon has to be removed, if it doesn't exist its w/e
	while (!ext.empty()) {
		if (ext[ext.size() - 1] == ';') {
			ext = ext.substr(0, ext.size() - 1);
		}

		// not checking for duplicates as well
		if (!ext.empty()) {
			_types["." + ext] = mimeType;
		}

		ext.clear();
		iss >> ext;
	}
}

bool	MimeTypes::load(const std::string& filepath) {

	// not decided yet on how to log that
	std::ifstream file(filepath.c_str());
	if (!file.is_open()) {
		std::cerr << "MimeTypes: failed to open " << filepath << std::endl;
		return (false);
	}

	_types.clear();
	_lastMimeType.clear();

	std::string	line;
	while (std::getline(file, line)) {
		parseLine(line);
	}

	file.close();
	_initialized = true;

	return (true);
}

// verify if the mime type is text or not, not even sure I'll use it
// depending on the return value here I might want to build my response in a way or in another (std::string or std::vector<char> -> to stock bytes instead of text)
bool	MimeTypes::isTextType(const std::string& mimeType) {

	if (mimeType.compare(0, 5, "text/") == 0)
		return (true);

	if (mimeType == "application/json" ||
		mimeType == "application/javascript" ||
		mimeType == "application/xml" ||
		mimeType == "application/xhtml+xml" ||
		mimeType == "application/x-www-form-urlencoded")
		return (true);

	return (false);
}
