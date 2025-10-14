#include "TcpServer.hpp"
#include "colors.h"

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
        throw std::runtime_error(std::string("bind: ") + strerror(errno));

    }

    status = listen(_socket, _request);
    if (status != 0) {
        freeaddrinfo(res);
        throw std::runtime_error(std::string("listen: ") + strerror(errno));

    }
}

void TcpServer::closeServer() {
    std::cout << "====== Closing Server ======" << std::endl;;
    freeaddrinfo(res);
    close(_socket);
    close (_clientFd);
    exit(0);
}

int TcpServer::getPort() const {
    return _port;
}

void TcpServer::acceptClient() {
    _socketAddress_len = sizeof(_socketAddress);
    _clientFd = accept(_socket, (struct sockaddr *)&_socketAddress, &_socketAddress_len);

    if (_clientFd == -1) {
        freeaddrinfo(res);
        throw std::runtime_error(std::string("accept: ") + strerror(errno));
    }

    std::cout << "Accepted new connection on socket: " << _clientFd << std::endl;
}