#include "Connection.hpp"

Connection::Connection() : _clientFd(-1), _ip(), _port(-1), _state(READING_REQUEST), _buffer(), _bufferLenght(-1), _keepAlive(true) {}

Connection::Connection(int& clientFd, std::string& ip, int& port) : _clientFd(clientFd), _ip(ip), _port(port), _state(READING_REQUEST), _buffer(), _bufferLenght(-1), _keepAlive(true) {}

Connection::~Connection() {
	(void) _clientFd;
	(void) _ip;
	(void) _port;
	(void) _state;
	(void) _buffer;
	(void) _bufferLenght;
	(void) _keepAlive;
	// /!\ do not close _clientFd, it should be managed by EventLopp
}

const std::string&	Connection::getIP(void) const {
	return (_ip);
}
