#include "Connection.hpp"

Connection::Connection() : _clientFd(-1), _state(READING_REQUEST), _buffer(), _bufferLenght(-1) {}

Connection::Connection(int clientFd) : _clientFd(clientFd), _state(READING_REQUEST), _buffer(), _bufferLenght(-1) {}

Connection::~Connection() {
	(void) _clientFd;
	(void) _state;
	(void) _buffer;
	(void) _bufferLenght;
	// /!\ do not close _clientFd, it should be managed by EventLopp
}
