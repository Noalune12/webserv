#include "TcpServer.hpp"
#include <iostream>
#include "colors.h"

int main() {
    try {
        TcpServer server("127.0.0.1", 7000);
        std::cout << GREEN "\nWelcome to my server" RESET << std::endl;
        std::cout << "Listening on port: " << RED << server.getPort() << RESET << std::endl;
        server.acceptClient();
    } catch (std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    return 0;
}