#ifndef CONFIGINHERITOR_HPP
# define CONFIGINHERITOR_HPP

# include <map>

# include "Tokenizer.hpp"

struct globalDir {
	std::map<int, std::string> errPage;
	double bodySize;
};

struct listenDirective {
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
	std::string returnPath;
	int returnStatus;
};

struct server {
	std::vector<listenDirective> lis;
	std::vector<std::string> serverName;
	std::string root;
	std::vector<std::string> index;
	std::vector<location> loc;
	allowMeth methods;
	bool autoIndex;
	std::string uploadTo;
	std::string returnPath;
	int returnStatus;

	std::map<int, std::string> errPage;
	double bodySize;
	bool	isRunning;

	server() : lis(), serverName(), root(), index(), loc(), methods(), autoIndex(false), uploadTo(), errPage(), bodySize(0), isRunning(false) {}
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
		std::vector<server>&	getServers(void);
		globalDir& getGlobalDir(void);
		void printContent() const;
};

#endif
