#include "MimeTypes.hpp"
#include <iostream>

int main() {

	std::cout << "--- getType ---" << std::endl;
	std::cout << "html			-> " << MimeTypes::getType("html") << std::endl;
	std::cout << ".html			-> " << MimeTypes::getType("index.html") << std::endl;
	std::cout << "index.html		-> " << MimeTypes::getType("index.html") << std::endl;
	std::cout << "/var/www/index.html	-> " << MimeTypes::getType("/var/www/index.html") << std::endl;

	std::cout << "\n--- Autres types ---" << std::endl;
	std::cout << "css		-> " << MimeTypes::getType("css") << std::endl;
	std::cout << "style.css	-> " << MimeTypes::getType("style.css") << std::endl;
	std::cout << "script.js	-> " << MimeTypes::getType("script.js") << std::endl;
	std::cout << "image.png	-> " << MimeTypes::getType("image.png") << std::endl;
	std::cout << "photo.jpg	-> " << MimeTypes::getType("photo.jpg") << std::endl;

	std::cout << "\n--- Extensions inconnues ---" << std::endl;
	std::cout << "file.xyz	-> " << MimeTypes::getType("file.xyz") << std::endl;
	std::cout << "noextension	-> " << MimeTypes::getType("noextension") << std::endl;

	std::cout << "\n--- getExtension ---" << std::endl;
	std::cout << "index.html		-> \"" << MimeTypes::getExtension("index.html") << "\"" << std::endl;
	std::cout << "/path/to/file.css	-> \"" << MimeTypes::getExtension("/path/to/file.css") << "\"" << std::endl;
	std::cout << "noextension		-> \"" << MimeTypes::getExtension("noextension") << "\"" << std::endl;
	std::cout << ".hidden			-> \"" << MimeTypes::getExtension(".hidden") << "\"" << std::endl;

	std::cout << "\n--- isTextType ---" << std::endl;
	std::cout << "text/html:		" << (MimeTypes::isTextType("text/html") ? "true" : "false") << std::endl;
	std::cout << "application/json:	" << (MimeTypes::isTextType("application/json") ? "true" : "false") << std::endl;
	std::cout << "image/png:		" << (MimeTypes::isTextType("image/png") ? "true" : "false") << std::endl;

	return (0);
}
