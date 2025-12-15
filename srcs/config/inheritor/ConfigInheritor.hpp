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
# include <map>
# include <string>
# include "Tokenizer.hpp"

struct globalDir {
	std::map<int, std::string> errPage;
	double bodySize; // en kilo octect
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

// struct ret {
// 	int code;
// 	std::string url; // change name
// };

struct location {
	std::string path;
	std::string alias;
	std::string cgiPath;
	std::string cgiExt;

	std::map<int, std::string> errPage;
	double bodySize; // en kilo octect
	std::string root;
	std::vector<std::string> index;
	allowMeth methods;
	bool autoIndex;
	std::string uploadTo;
	std::map<int, std::string> ret;
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
	std::map<int, std::string> ret;

	std::map<int, std::string> errPage;
	double bodySize; // en kilo octect
};

class ConfigInheritor {

	private:
		globalDir _globalDir;
		std::vector<server> _server;

	public:
		ConfigInheritor();
		ConfigInheritor(Tokenizer& tokens);
		~ConfigInheritor();

		void getGlobalDir(std::vector<std::pair<std::string, std::vector<std::string> > >	globalDir);
		void getServer(std::vector<Context>	context);
		void getLocation(std::vector<Context> loc, server& server); // 

		void getErrPageFromGlobal(server& server);
		void getErrPageFromServer(server& server, location& location);
		void getReturnFromServer(server& server, location& location);
		void printContent() const;

};

#endif
