#include "TcpServer.hpp"
#include "colors.h"
#include <sstream>
#include <fcntl.h>
#include <cstdio>
#include <algorithm>

TcpServer::TcpServer(std::string ip, int port) : _ipAddress(ip), \
    _port(port), _socketAddress(), \
    _socketAddress_len(sizeof(_socketAddress)), \
    _socket(), _clientFd(), _request(10) {
    startServer();
    std::cout << "IP = " << _ipAddress << "\nport = " << _port << std::endl;
}

TcpServer::~TcpServer() {
    closeServer();
}

void TcpServer::startServer() {
    std::cout << BLUE << "Preparing Server Info...\n" << RESET << std::endl;

    // hints = {0};
    memset(&hints, 0, sizeof hints); // how to do in CPP
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    int status = getaddrinfo(_ipAddress.c_str(), 0, &hints, &res);
    if (status != 0) {
        std::string error = gai_strerror(status);
        throw std::runtime_error("getaddrinfo: " + error);
    }

    for (r = res; r; r = r->ai_next) {
        if (r->ai_family == AF_INET) {
            _socketAddress = *(struct sockaddr_in*)(r->ai_addr);
            _socketAddress_len = r->ai_addrlen;
            break;
        }
    }

    std::cout << YELLOW "= SOCKET ADDRESS JUST AFTER LOOP =" RESET << "\ns_addr = " << _socketAddress.sin_addr.s_addr \
        << "\nsin_family = " << _socketAddress.sin_family << "\nsin_port = " << _socketAddress.sin_port << "\n" << std::endl;
    
    _socketAddress.sin_family = AF_INET;
    // _socketAddress.sin_addr.s_addr =_ipAddress;
    _socketAddress.sin_port = htons(_port);

    std::cout << YELLOW "= SOCKET ADDRESS AFTER OTHER INIT =" RESET << "\ns_addr = " << _socketAddress.sin_addr.s_addr \
            << "\nsin_family = " << _socketAddress.sin_family << "\nsin_port = " << _socketAddress.sin_port << "\n" << std::endl;

    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0) {
        freeaddrinfo(res);
        throw std::runtime_error(std::string("socket: ") + strerror(errno));
    }
    std::cout << "Server socket created with fd = " << _socket << std::endl;

    
    status = bind(_socket, (struct sockaddr *)&_socketAddress, sizeof(_socketAddress));
    if (status != 0) {
        freeaddrinfo(res);
        close(_socket);
        throw std::runtime_error(std::string("bind: ") + strerror(errno));
    }

    status = listen(_socket, _request);
    if (status != 0) {
        freeaddrinfo(res);
        close(_socket);
        throw std::runtime_error(std::string("listen: ") + strerror(errno));
    }

    // ADDED
    // fcntl(_socket, F_SETFL, O_NONBLOCK); //protect ??
    // _epollFd = epoll_create(1); //protect ??
    // _ev.events = EPOLLIN; // accept
    // _ev.data.fd = _socket;
    // if(epoll_ctl(_epollFd, EPOLL_CTL_ADD, _socket , &_ev))
    // {
    //     // fprintf(stderr, "Failed to add file descriptor to epoll\n");
    //     close(_epollFd); // and everything else
    //     throw std::runtime_error("epoll ctl error");
    // }

}

void TcpServer::closeServer() {
    std::cout << RED "====== Closing Server ======" RESET << std::endl;;
    freeaddrinfo(res);
    close(_socket);
    close (_clientFd);
    // when to close epoll fds registered
    // exit(0);
}

int TcpServer::getPort() const {
    return _port;
}

template <typename T>
std::string NumberToString ( T N )
{
    std::ostringstream ss;
    ss << N;
    return ss.str();
}

void TcpServer::acceptClient() {
    // struct epoll_event events[10]; // how to chose max events
    // while (1) {
    //     int n = epoll_wait(_epollFd, events, 10, 2000);
    //     if (n == -1) {
    //         //error epoll wait
    //         break;
    //     } else if (n == 0) {
    //         std::cout << "[Server] Waiting ..." << std::endl;
    //     }
    //     for (int i = 0; i < n; ++i) {
    //         int fd = events[i].data.fd;
    //         if (fd == _socket) { // new connection
    //             //server
    //             _socketAddress_len = sizeof(_socketAddress);
    //             _clientFd = accept(_socket, (struct sockaddr *)&_socketAddress, &_socketAddress_len);

    //             if (_clientFd == -1) {
    //                 throw std::runtime_error(std::string("accept: ") + strerror(errno));
    //             }

    //             fcntl(_clientFd, F_GETFL, O_NONBLOCK);

    //             // add client to epoll
    //             struct epoll_event client_ev;
    //             client_ev.events = EPOLLIN | EPOLLET;
    //             client_ev.data.fd = _clientFd;
    //             _clientsFd.push_back(_clientFd);
    //             epoll_ctl(_epollFd, EPOLL_CTL_ADD, _clientFd, &client_ev);

    //             std::cout << "Accepted new connection on socket: " << _clientFd << std::endl;

    //         } else if (events[i].events & EPOLLIN) {
    //             std::cout << "read data from socket" << std::endl;
    //             char buffer[1024];
    //             memset(buffer, 0, 1024);
    //             int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    //             if (bytes <= 0) {
    //                 if (bytes == 0) {
    //                     std::cout << "[Server] Client disconnected fd=" << fd << std::endl;
    //                 }
    //                 else
    //                     perror("recv");

    //                 epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, 0);
    //                 close(fd);
    //                 for (std::vector<int>::iterator it = _clientsFd.begin(); it != _clientsFd.end(); it++) {
    //                     if (*it == fd) {
    //                         _clientsFd.erase(it);
    //                         break;
    //                     }
    //                 }
    //             } else {
    //                 std::string req = buffer;
    //                 buffer[bytes] = '\0';
    //                 std::cout << "Message recieved from [" << fd << "]: " << buffer << std::endl;
    //                 std::string msg = "From server: message well recieved\n";
    //                 int bytesSent = send(_clientFd, msg.c_str(), msg.size(), 0);
    //                 if (bytesSent < 0) 
    //                     throw std::runtime_error("send to client " + NumberToString(_clientFd) + strerror(errno));
    //                 std::string sendMsg = "[" + NumberToString(fd) + "] = " + buffer;
    //                 for (size_t j = 0; j < _clientsFd.size(); j++) {
    //                     int other = _clientsFd[j];
    //                     if (other == fd)
    //                         continue;
    //                     ssize_t bytesSent = send(other, sendMsg.c_str(), sendMsg.size(), 0);
    //                     if (bytesSent == -1) {
    //                         //error
    //                     }
    //                 }
    //                 if (req.find("\r\n\r\n") != std::string::npos) {
    //                     epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, 0);
    //                     close(fd);
    //                 }

    //             }
    //         } 
    //     }
    // }


    while (1) {
        _socketAddress_len = sizeof(_socketAddress);
        std::cout << "\nWaiting for a new connection" << std::endl;

        _clientFd = accept(_socket, (struct sockaddr *)&_socketAddress, &_socketAddress_len);

        if (_clientFd == -1) {
            throw std::runtime_error(std::string("accept: ") + strerror(errno));
        }

        std::cout << "Accepted new connection on socket: " << _clientFd << std::endl;

        getRequest();
        close (_clientFd);
    }
}

#include <sstream>

void TcpServer::getRequest() {
    ssize_t bytesRecieved;
    char buffer[BUFSIZ];
    memset(buffer, 0, sizeof(buffer));
    _req.clear();
    while ((bytesRecieved = recv(_clientFd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        if (bytesRecieved < 0) 
            throw std::runtime_error(std::string("recv: ") + strerror(errno));
        if (bytesRecieved == 0) 
            throw std::runtime_error("recv: client " + NumberToString(_clientFd) + " closed connection");
        _req += buffer;

        if (_req.find("\r\n\r\n") != std::string::npos || _req.find("\0") != std::string::npos)
            break;

        memset(buffer, 0, sizeof(buffer));
    }
    // size_t pos = _req.find("\r\n\r\n");
    // if (pos != std::string::npos) {
    //     _req = _req.substr(0, pos);
    // }
    std::cout << YELLOW "MESSAGE RECIEVED\n\'" RESET << _req << YELLOW "\'MESSAGE END" RESET << std::flush << std::endl;

    checkRequestSem();
    // send the html if GET
    // std::string msg = "message well recieved\n";
    // int bytesSent = send(_clientFd, msg.c_str(), msg.size(), 0);
    // if (bytesSent < 0) 
    //     throw std::runtime_error("send to client " + NumberToString(_clientFd) + strerror(errno));
    // std::cout << GREEN "\nMESSAGE SENT" RESET << std::endl;
}

void printWithoutR(std::string what, std::string line) {
    std::string l;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] != '\r')
            l.push_back(line[i]);
    }
    std::cout << what <<" = \'" << l << "\' -" << std::endl;
}

bool TcpServer::checkRequestSem() {
    if (_req.empty()) { // how to test
        send400();
        std::cout << "error empty request" << std::endl;
        return false;
    }

    // extracts request line, headers and body
    std::string requestLine, headers, body;
    size_t index = _req.find("\r\n");
    if (index == std::string::npos || index == 0) {
        send400();
        std::cout << "error with request line" << std::endl;
        return false;  
    }
    std::cout << "INDEX of r n = " << index << std::endl;
    requestLine = _req.substr(0, index);
    _req = _req.substr(index + 2);
    printWithoutR("REQUEST LINE", requestLine);
    printWithoutR("REMAINING REQUEST", _req);

    index = _req.find("\r\n\r\n");
    if (index == std::string::npos || index == 0) {
        send400();
        std::cout << "error no header or final WS" << std::endl;
        return false;  
    }
    headers = _req.substr(0, index + 1);
    _req = _req.substr(index + 4);
    printWithoutR("HEADERS", headers);
    printWithoutR("REMAINING REQUEST", _req);

    body = _req;
    printWithoutR("BODY", body);

    // ANALYSE REQUEST LINE
    std::string method, uri, http, host;

    // method
    index = requestLine.find(' ');
    if (index == std::string::npos || index == 0) {
        send400();
        std::cout << "error with request line" << std::endl;
        return false;
    }
    method = requestLine.substr(0, index);
    std::string remain = requestLine.substr(index + 1, requestLine.length());
    printWithoutR("METHOD", method);
    printWithoutR("REMAIN", remain);

    // uri
    index = remain.find(' ');
    if (index == std::string::npos || index == 0) {
        send400();
        std::cout << "error with request line" << std::endl;
        return false;
    }
    uri = remain.substr(0, index);
    remain = remain.substr(index + 1, remain.length());
    printWithoutR("URI", uri);
    printWithoutR("REMAIN", remain);

    //http
    if (remain.empty()) {
        send400();
        std::cout << "error with request line" << std::endl;
        return false;
    }
    http = remain;
    printWithoutR("HTTP", http);

    std::cout << "request line is ok" << std::endl;

    if (!checkRequestLine(method, uri, http) || !checkHeaders(headers)) {
        return false;
    }
    // std::stringstream ss(_req);
    // std::string line;
    // while (std::getline(ss, line)) {

    // }

    // there can only be one host, content-length and user-agent
    // if transfer-encoding: chunked no content-length -> body finish with \r\n\r\n and then next request
    // compare body with content-length

    // if (!checkRequestLine(method, uri, http))
    //     return false;
    return true;
}

bool hasWS(std::string s) {
    for (size_t i = 0; i < s.length(); i++) {
        if (isspace(s[i]))
            return true;
    }
    return false;
}

bool TcpServer::checkHeaders(std::string headers) {
    _headers.clear();
    std::stringstream ss(headers);
    std::string line;
    while (std::getline(ss, line)) {
        if (line[line.length() - 1] != '\r') {
            send400();
            std::cout << "error with headers no \\r at the end" << std::endl;
            return false;
        }
        line = line.substr(0, line.length() - 1);
        size_t index = line.find(": ");
        std::cout << "index : = " << index << std::endl;
        std::cout << "HEADER = " << line << std::endl;
        if (index == 0 || index == std::string::npos || index == line.length() - 2) {
            send400();
            std::cout << "error with headers no : or no header name or content" << std::endl;
            return false;
        }
        std::string name = line.substr(0, index);
        std::string content = line.substr(index + 2);
        std::cout << "NAME, CONTENT FOR HEADER = " << name << ", " << content << std::endl;
        if (hasWS(name) || hasWS(content)) {
            send400();
            std::cout << "error with headers whitespaces" << std::endl;
            return false;  
        }
        if ((name == "Host" && _headers.find("Host") != _headers.end()) 
                || (name == "User-Agent" && _headers.find("User-Agent") != _headers.end())
                || (name == "Content-Length" && _headers.find("Content-Length") != _headers.end())) {
            send400();
            std::cout << "error with duplicate headers : " << name << std::endl;
            return false;  
        }
        _headers[name] = content;
    }

    std::cout << "\nHEADER MAP\n" << std::endl;
    std::map<std::string, std::string>::iterator it = _headers.begin();
    for (; it != _headers.end(); it++) {
        std::cout << "[" << it->first << ", " << it->second << "]" << std::endl;
    }

    return true;
}

bool TcpServer::checkRequestLine(std::string& method, std::string& uri, std::string& http) {
    // if (_req.empty()) {
    //     send400();
    //     std::cout << "error no headers" << std::endl;
    //     return false;
    // }
    if (method != "GET" && method != "POST" && method != "DELETE") { //what if method is not accepted
        send400();
        std::cout << "error with method" << std::endl;
        return false;
    }
    if (uri[0] != '/') { //what if uri is not defined
        send400();
        std::cout << "error with uri" << std::endl;
        return false;
    }
    if (http != "HTTP/1.1") {
        send400();
        std::cout << "error with http" << std::endl;
        return false;
    }
    return true;
}

void TcpServer::send400() {
    // find if error page 400 in config file
    std::string body =
        "<html>\n"
        "<head><title>400 Bad Request</title></head>\n"
        "<body><h1>400 Bad Request</h1>\n"
        "<p>Your request was malformed.</p>\n"
        "</body>\n"
        "</html>\n";

    std::stringstream ss;
    ss << body.size();
    std::string bodySize = ss.str();

    std::string response =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + bodySize + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;

    send(_clientFd, response.c_str(), response.size(), 0);

}