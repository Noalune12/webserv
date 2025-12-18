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
	double bodySize;
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

struct location {
	std::string path;
	std::string alias;
	std::string cgiPath;
	std::string cgiExt;

	std::map<int, std::string> errPage;
	double bodySize;
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
	double bodySize;
};

class ConfigInheritor {

	private:
		globalDir _globalDir;
		std::vector<server> _server;
		
		void getGlobalDir(PairVector	globalDir);
		void getServer(std::vector<Context>	context);
		void getLocation(std::vector<Context> loc, server& server); // 
	
		void getErrPageFromGlobal(server& server);
		void getErrPageFromServer(server& server, location& location);
		void getReturnFromServer(server& server, location& location);
		void printContent() const;
	
		template <typename T>
		void setErrorPage(PairVector::iterator& it, T& t);
		template <typename T>
		void setBodySize(PairVector::iterator& it, T& t);
		template <typename T>
		void setIndex(PairVector::iterator& it, T& t);
		template <typename T>
		void setReturn(PairVector::iterator& it, T& t);
		
		void setMethods(PairVector::iterator& it, allowMeth& methods);
		void setServerName(PairVector::iterator& it, std::vector<std::string>& serverName);
		void setListen(PairVector::iterator& it, server& s);

	public:
		ConfigInheritor();
		~ConfigInheritor();

		void inherit(Tokenizer& tokens);
};

#endif
