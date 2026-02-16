#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <map>

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

        // SET UP
        std::string _req;
        std::vector<server>	_servers;
        globalDir _globalDir;
        int _serverPort;
        std::string _serverIp;

        // HEADERS PARSING
        std::string _headersStr;
        std::string _requestLine;
        std::string _serverName;

        // BODY
        double  _chunkSize;
        int     _chunkState;
        int     _multipartState;
        Multipart _multiTemp;
        std::string _multipartBoundary;
        std::vector<Multipart> _multipartContent;

        // POST
        double _failedUpload;
        double _totalUpload;
        std::string _postExt;
        std::string _postFilename;
        std::string _postUploadDir;
        std::string _getPath;

        // RESPONSE
        bool _indexFound;


        // PARSING
        bool extractRequestInfo();
        bool extractRequestLineInfo(std::string& method, std::string& uri, std::string& http);
        bool checkRequestLine(const std::string& method, const std::string& uri, const std::string& http);
        bool checkHeaders();
        bool hostChecker();
        void findServer();
        void findLocation();

        // BODY PARSING
        bool bodyChecker();
        bool checkChunked();
        void checkMultipart(const std::string& content);
        bool isMultipartCarac(const std::string &boundary);
        bool getChunkSize();
        bool handleMultipartHeader();
        bool checkMultipartHeader();
        bool checkContentDisposition(const std::string& content);
        bool checkContentType(const std::string& content);
        bool checkLastBoundary(int endIndex, const std::string &finalBoundary);

        // HANDLER
        void methodGetHandler();
        void methodDeleteHandler();
        void methodPostHandler();
        std::string getPath(const std::string& folder);
        std::string getPath(const std::string& folder, std::string& file);
        bool readFile(const std::string& path, struct stat buf);
        bool handleAutoindex(const std::string& dirPath);
        std::string getDirectory();
        bool deleteFolder(const std::string& path);
        bool findUploadDir();
        bool checkFilename(const std::string &filename);
        bool createFileName(const std::string &filename);
        void printFilename() const;
        void handleMultipart();

        // UTILS
        std::string trimOws(const std::string& s);
        std::string trimFirstCRLF(const std::string& s);
        bool hasWS(const std::string& line) const;
        bool isOnlyDigits(const std::string& line) const;
        void printWithoutR(const std::string& what, const std::string& line) const;
        bool convertHexa(std::string& hex);

    public:
        Request();
        Request(std::vector<server>	servers, globalDir globalDir);
        ~Request();

        // RESPONSE
        bool err;
        int status;
        std::string			htmlPage;

        // RL AND HEADERS
        std::string _method;
        std::string _uri;
        std::string _body;
        std::map<std::string, std::string> _headers;
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

        // BODY
        bool chunkRemaining;
        bool isChunked;
        std::string chunk;
        bool isMultipart;
        std::string fullBody;
        bool multipartRemaining;
        bool remainingBody;

        // POST
        std::vector<FilesPost> _uplaodFiles;

        // PARSING
        bool isCRLF(const std::string& request);
        void checkRequestSem(std::string request);
        void checkRequestContent();
        bool checkBodySize(const std::string &body);


        // MULTIPART
        bool parseMultipart();

        // CHUNK PARSING
        void parseChunk();

        // HANDLER
        void methodHandler();

        // UTILS
        void clearPreviousRequest();
        void setServerInfo(const int& port, const std::string& ip);
        void findErrorPage(int code, const std::string& root, const std::map<int, std::string>& errPage);
};


#endif
