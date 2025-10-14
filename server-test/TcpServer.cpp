#include "TcpServer.hpp"
#include "colors.h"
#include <sstream>

TcpServer::TcpServer(std::string ip, int port) : _ipAddress(ip), \
    _port(port), _incomingMsg(), _socketAddress(), \
    _socketAddress_len(sizeof(_socketAddress)), _serverMessage(), \
    _socket(), _clientFd(), _request(10) {
    startServer();
    std::cout << "IP = " << _ipAddress << "\nport = " << _port << std::endl;
    (void)_incomingMsg;
    (void)_socketAddress_len;
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
}

void TcpServer::closeServer() {
    std::cout << RED "====== Closing Server ======" RESET << std::endl;;
    freeaddrinfo(res);
    close(_socket);
    close (_clientFd);
    // exit(0);
}

int TcpServer::getPort() const {
    return _port;
}

void TcpServer::acceptClient() {
    _socketAddress_len = sizeof(_socketAddress);
    _clientFd = accept(_socket, (struct sockaddr *)&_socketAddress, &_socketAddress_len);

    if (_clientFd == -1) {
        freeaddrinfo(res);
        close(_socket);
        close (_clientFd);
        throw std::runtime_error(std::string("accept: ") + strerror(errno));
    }

    std::cout << "Accepted new connection on socket: " << _clientFd << std::endl;

    getRequest();
}

template <typename T>
std::string NumberToString ( T N )
{
    std::ostringstream ss;
    ss << N;
    return ss.str();
}

void TcpServer::getRequest() {
    ssize_t bytesRecieved;
    std::string req;
    char buffer[BUFSIZ];
    memset(buffer, 0, sizeof(buffer));

    while ((bytesRecieved = recv(_clientFd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        if (bytesRecieved < 0) 
            throw std::runtime_error(std::string("recv: ") + strerror(errno));
        if (bytesRecieved == 0) 
            throw std::runtime_error("recv: client " + NumberToString(_clientFd) + " closed connection");
        req += buffer;

        if (req.find("\r\n\r\n") != std::string::npos)
            break;

        memset(buffer, 0, sizeof(buffer));
    }
    size_t pos = req.find("\r\n\r\n");
    if (pos != std::string::npos) {
        req = req.substr(0, pos);
    }
    std::cout << YELLOW "MESSAGE RECIEVED\n'" RESET << req << YELLOW "'MESSAGE END" RESET << std::endl;

    // get line by line in a vector ? (\n)

    std::string msg = "message well recieved\n";
    int bytesSent = send(_clientFd, msg.c_str(), msg.size(), 0);
    if (bytesSent < 0) 
        throw std::runtime_error("send to client " + NumberToString(_clientFd) + strerror(errno));
    std::cout << GREEN "\nMESSAGE SENT" RESET << std::endl;
}

