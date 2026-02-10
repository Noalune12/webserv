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

enum BodyState {
    GETTING_BODY,
    END_BODY,
};

enum MultipartState {
    GETTING_FIRST_BOUNDARY,
    WAITING_BOUNDARY,
    IS_HEADERS,
    IS_BODY,
    IS_MULTI_END,
};

struct Multipart {
    std::map<std::string, std::string>  headers;
    std::string                         body;
    std::string                         filename;
    std::string                         name;
};

struct FilesPost {
    std::string filename;
    std::string location;
};

class Request {
    private:
        std::string _req;
        std::string _headersStr;
        std::string _requestLine;
        std::vector<server>	_servers;
        globalDir _globalDir;

        std::string _serverName;

        double _chunkSize;
        int _chunkState;
        int _multipartState;

        int _serverPort;
        std::string _serverIp;

        Multipart _multiTemp;
        double _failedUpload;
        double _totalUpload;

    public:
        Request();
        Request(std::vector<server>	servers, globalDir globalDir);
        ~Request();

        bool err;
        int status;
        bool chunkRemaining;
        bool _isChunked;
        std::string _method;
        std::string _uri;
        // _http
        std::string _body;
        std::map<std::string, std::string> _headers;
        std::string			htmlPage;
        std::string _chunk;
        std::string _trailing;
        bool _keepAlive;
        server *_reqServer;
	    location *_reqLocation;
        std::string _version;
        bool _cgi;
        bool _return;
        std::string _scriptPath;
        std::string _queryString;
        std::string _returnPath;
        std::string _postExt;
        std::string _postFilename;
        std::string _postUploadDir;
        std::string _getPath;
        bool _indexFound;
        bool _isMultipart;
        std::string _multipartBoundary;
        std::vector<Multipart> _multipartContent;
        std::string _fullBody;
        bool _multipartRemaining;
        std::vector<FilesPost> _uplaodFiles;

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

        // MULTIPART
        void checkMultipart(std::string content);
        bool isMultipartCarac(std::string &boundary);
        bool parseMultipart();

        // CHUNK PARSING
        void parseChunk();
        bool getChunkSize();

        // HANDLER
        void methodHandler();
        void methodGetHandler();
        void methodDeleteHandler();
        void methodPostHandler();
        std::string getPath(std::string folder);
        std::string getPath(std::string folder, std::string file);
        bool readFile(std::string path, struct stat buf);
        bool handleAutoindex(std::string dirPath);
        std::string getDirectory();
        bool deleteFolder(std::string path);
        bool findUploadDir();
        bool checkFilename(std::string &filename);
        void createFileName(std::string &filename);
        void printFilename() const;

        // UTILS
        std::string trimOws(const std::string& s);
        std::string trimFirstCRLF(const std::string& s);
        void findErrorPage(int code, std::string path, std::map<int, std::string> errPage);
        void clearPreviousRequest();
        bool hasWS(const std::string& line) const;
        bool isOnlyDigits(const std::string& line) const;
        void printWithoutR(std::string what, std::string line) const;
        void setServerInfo(const int& port, const std::string& ip);
};


#endif
