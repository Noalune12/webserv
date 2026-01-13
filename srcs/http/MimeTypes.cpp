#include <algorithm>
#include <cctype>

#include "MimeTypes.hpp"

std::map<std::string, std::string>	MimeTypes::_types;
bool								MimeTypes::_initialized = false;

MimeTypes::MimeTypes(void) {}

MimeTypes::~MimeTypes(void) {}

void	MimeTypes::initializeTypes(void) {

	if (_initialized)
		return ;

	_types[".html"]  = "text/html";
	_types[".htm"]   = "text/html";
	_types[".css"]   = "text/css";
	_types[".txt"]   = "text/plain";
	_types[".xml"]   = "text/xml";
	_types[".csv"]   = "text/csv";

	_types[".js"]    = "application/javascript";

	_types[".json"]  = "application/json";

	_types[".png"]   = "image/png";
	_types[".jpg"]   = "image/jpeg";
	_types[".jpeg"]  = "image/jpeg";
	_types[".gif"]   = "image/gif";
	_types[".bmp"]   = "image/bmp";
	_types[".svg"]   = "image/svg+xml";
	_types[".webp"]  = "image/webp";

	_types[".mp3"]   = "audio/mpeg";
	_types[".wav"]   = "audio/wav";

	_types[".mp4"]   = "video/mp4";
	_types[".webm"]  = "video/webm";
	_types[".avi"]   = "video/x-msvideo";

	_types[".pdf"]   = "application/pdf";
	_types[".zip"]   = "application/zip";
	_types[".gz"]    = "application/gzip";
	_types[".tar"]   = "application/x-tar";

	_initialized = true;
}

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

	initializeTypes();

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
