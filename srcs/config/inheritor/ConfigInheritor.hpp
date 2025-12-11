#ifndef CONFIGINHERITOR_HPP
# define CONFIGINHERITOR_HPP

/*
 * Gestion de l'héritage des directives de configuration
 *
 * Responsabilités:
 * - Applique l'héritage des directives: global -> server -> location
 * - Les directives définies au niveau global sont héritées par tous les blocs server
 * - Les directives définies dans server sont héritées par tous les blocs location
 * - Les directives locales surchargent les directives héritées (override)
 */

# include <vector>
# include <string>
# include "Tokenizer.hpp"

struct errorPage {
	std::vector<int> code;
	std::string uri;
};

struct clMaxBSize {
	int size;
	char type;
};

struct globalDir {
	errorPage errPage;
	clMaxBSize bodySize; 
};

struct listen {
	std::string ip;
	int port;
};

struct allowMeth {
	bool get;
	bool del;
	bool post;
};

struct ret {
	int code;
	std::string url;
};

struct location {
	std::string path;
	std::string alias;
	std::string cgiPath;
	std::string cgiExt;

	errorPage errPage;
	clMaxBSize bodySize;
	std::string root;
	std::vector<std::string> index;
	allowMeth methods;
	bool autoIndex;
	std::string uploadTo;
	ret r;
};

struct server {
	std::vector<listen> lis;
	std::vector<std::string> serverName;
	std::string root;
	std::vector<std::string> index;
	std::vector<location> loc;
	allowMeth methods;
	bool autoIndex;
	std::string uploadTo;
	ret r;

	errorPage errPage;
	clMaxBSize bodySize; 
};

class ConfigInheritor {

	private:
		globalDir _globalDir;
		std::vector<server> _server;

	public:
		ConfigInheritor();
		ConfigInheritor(Tokenizer& tokens);
		~ConfigInheritor();

};

#endif
