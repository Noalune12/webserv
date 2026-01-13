#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <map>
# include <vector>
# include <string>
# include "ConfigInheritor.hpp"

class Request {
    private:
        std::string _req;
        std::string _headersStr;
        std::string _requestLine;
        std::vector<server>	_servers;
        globalDir _globalDir;
    
    public:
        Request();
        Request(std::vector<server>	_servers, globalDir globalDir);
        ~Request();

        bool err;
        int status;
        std::string _method;
        std::string _uri;
        std::string _body;
        std::map<std::string, std::string> _headers;
        std::string			htmlPage;

        void checkRequestSem(std::string request);
        bool extractRequestInfo();
        bool extractRequestLineInfo(std::string& method, std::string& uri, std::string& http);
        bool checkRequestLine(std::string& method, std::string& uri, std::string& http);
        bool checkHeaders();
        void checkRequestContent();
        std::string trimOws(const std::string& s);
        std::string lowerString(const std::string& s);
        void findErrorPage(int code, std::string path, std::map<int, std::string> errPage);

};


#endif
