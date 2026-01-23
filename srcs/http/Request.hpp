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
        server _reqServer;
	    location _reqLocation;

        double _chunkSize;
        int _chunkState;

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

        void checkRequestSem(std::string request);
        bool extractRequestInfo();
        bool extractRequestLineInfo(std::string& method, std::string& uri, std::string& http);
        bool checkRequestLine(std::string& method, std::string& uri, std::string& http);
        bool checkHeaders();
        void checkRequestContent();
        std::string trimOws(const std::string& s);
        std::string lowerString(const std::string& s);
        void findErrorPage(int code, std::string path, std::map<int, std::string> errPage);
        void isChunkEnd();
        void clearPreviousRequest();
        void parseChunk();
};


#endif
