#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


class TcpServer {
    public:
        TcpServer(std::string ip, int port);
        ~TcpServer();

        int getPort() const;
        void acceptClient();

    private:
        //getaddrinfo
        struct addrinfo hints; // Hints or "filters" for getaddrinfo()
        struct addrinfo *res;  // Result of getaddrinfo()
        struct addrinfo *r;    // Pointer to iterate on results

        std::string _ipAddress;
        int _port;
        long _incomingMsg;
        struct sockaddr_in _socketAddress;
        unsigned int _socketAddress_len;
        std::string _serverMessage;
        
        int _socket;
        int _clientFd;
        int _request;

        void startServer();
        void closeServer();
};

#endif