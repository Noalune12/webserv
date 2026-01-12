#ifndef REQUESTPARSING_HPP
# define REQUESTPARSING_HPP

# include <map>
# include <string>

class RequestParsing {
    private:
        std::string _req;
    
    public:
        RequestParsing();
        ~RequestParsing();

        bool err;
        int status;
        std::string _method;
        std::string _uri;
        std::string _body;
        std::map<std::string, std::string> _headers;

        void checkRequestSem(std::string request);
        bool checkRequestLine(std::string& method, std::string& uri, std::string& http);
        bool checkHeaders(std::string headers);
};


#endif
