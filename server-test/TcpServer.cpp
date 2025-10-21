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
    fcntl(_socket, F_SETFL, O_NONBLOCK); //protect ??
    _epollFd = epoll_create(1); //protect ??
    _ev.events = EPOLLIN; // accept
    _ev.data.fd = _socket;
    if(epoll_ctl(_epollFd, EPOLL_CTL_ADD, _socket , &_ev))
    {
        // fprintf(stderr, "Failed to add file descriptor to epoll\n");
        close(_epollFd); // and everything else
        throw std::runtime_error("epoll ctl error");
    }

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
    struct epoll_event events[10]; // how to chose max events
    while (1) {
        int n = epoll_wait(_epollFd, events, 10, 2000);
        if (n == -1) {
            //error epoll wait
            break;
        } else if (n == 0) {
            std::cout << "[Server] Waiting ..." << std::endl;
        }
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == _socket) { // new connection
                //server
                _socketAddress_len = sizeof(_socketAddress);
                _clientFd = accept(_socket, (struct sockaddr *)&_socketAddress, &_socketAddress_len);

                if (_clientFd == -1) {
                    throw std::runtime_error(std::string("accept: ") + strerror(errno));
                }

                fcntl(_clientFd, F_GETFL, O_NONBLOCK);

                // add client to epoll
                struct epoll_event client_ev;
                client_ev.events = EPOLLIN | EPOLLET;
                client_ev.data.fd = _clientFd;
                _clientsFd.push_back(_clientFd);
                epoll_ctl(_epollFd, EPOLL_CTL_ADD, _clientFd, &client_ev);

                std::cout << "Accepted new connection on socket: " << _clientFd << std::endl;

            } else if (events[i].events & EPOLLIN) {
                std::cout << "read data from socket" << std::endl;
                char buffer[1024];
                memset(buffer, 0, 1024);
                int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                    if (bytes == 0) {
                        std::cout << "[Server] Client disconnected fd=" << fd << std::endl;
                    }
                    else
                        perror("recv");

                    epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, 0);
                    close(fd);
                    for (std::vector<int>::iterator it = _clientsFd.begin(); it != _clientsFd.end(); it++) {
                        if (*it == fd) {
                            _clientsFd.erase(it);
                            break;
                        }
                    }
                } else {
                    std::string req = buffer;
                    buffer[bytes] = '\0';
                    std::cout << "Message recieved from [" << fd << "]: " << buffer << std::endl;
                    std::string msg = "From server: message well recieved\n";
                    int bytesSent = send(_clientFd, msg.c_str(), msg.size(), 0);
                    if (bytesSent < 0) 
                        throw std::runtime_error("send to client " + NumberToString(_clientFd) + strerror(errno));
                    std::string sendMsg = "[" + NumberToString(fd) + "] = " + buffer;
                    for (size_t j = 0; j < _clientsFd.size(); j++) {
                        int other = _clientsFd[j];
                        if (other == fd)
                            continue;
                        ssize_t bytesSent = send(other, sendMsg.c_str(), sendMsg.size(), 0);
                        if (bytesSent == -1) {
                            //error
                        }
                    }
                    if (req.find("\r\n\r\n") != std::string::npos) {
                        epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, 0);
                        close(fd);
                    }

                }
            } 
        }
    }


    // while (1) {
    //     _socketAddress_len = sizeof(_socketAddress);
    //     std::cout << "\nWaiting for a new connection" << std::endl;

    //     _clientFd = accept(_socket, (struct sockaddr *)&_socketAddress, &_socketAddress_len);

    //     if (_clientFd == -1) {
    //         throw std::runtime_error(std::string("accept: ") + strerror(errno));
    //     }

    //     std::cout << "Accepted new connection on socket: " << _clientFd << std::endl;

    //     getRequest();
    //     close (_clientFd);
    // }
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

