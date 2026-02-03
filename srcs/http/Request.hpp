#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <map>
# include <vector>
# include <string>
# include "ConfigInheritor.hpp"

enum ChunckState {
    GETTING_FIRST_SIZE,
    GETTING_SIZE,
    READING_BYTES,
    IS_END,
};


class Request {
    private:
        std::string _req;
        std::string _headersStr;
        std::string _requestLine;
        std::vector<server>	_servers;
        globalDir _globalDir;
        server *_reqServer;
	    location *_reqLocation;
        std::string _serverName;

        double _chunkSize;
        int _chunkState;

        int _serverPort;
        std::string _serverIp;

    public:
        Request();
        Request(std::vector<server>	servers, globalDir globalDir);
        ~Request();

        bool err;
        int status;
        bool chunkRemaining;
        std::string _method;
        std::string _uri;
        // _http
        std::string _body;
        std::map<std::string, std::string> _headers;
        std::string			htmlPage;
        std::string _chunk;
        std::string _trailing;
        bool _keepAlive;

        // PARSING
        void checkRequestSem(std::string request);

        bool extractRequestInfo();
        bool extractRequestLineInfo(std::string& method, std::string& uri, std::string& http);
        bool checkRequestLine(std::string& method, std::string& uri, std::string& http);
        bool checkHeaders();
        
        void checkRequestContent();

        bool hostChecker();
        void findServer();
        void findLocation();
        bool bodyChecker();

        // CHUNK PARSING
        void parseChunk();
        bool getChunkSize();

        // HANDLER
        void methodHandler();
        void methodGetHandler();
        std::string getPath(std::string folder);
        std::string getPath(std::string folder, std::string file);
        bool readFile(std::string path, struct stat buf, std::string errorPath);
        std::string getDirectory();
        bool deleteFolder(std::string path);

        // UTILS
        std::string trimOws(const std::string& s);
        void findErrorPage(int code, std::string path, std::map<int, std::string> errPage);
        void clearPreviousRequest();
        bool hasWS(const std::string& line) const;
        bool isOnlyDigits(const std::string& line) const;
        void printWithoutR(std::string what, std::string line) const;
        void setServerInfo(const int& port, const std::string& ip);
};


#endif
