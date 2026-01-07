#include "Connection.hpp"

Connection::Connection() : _clientFd(-1), _state(READING_REQUEST) {}

Connection::Connection(int clientFd) : _clientFd(clientFd), _state(READING_REQUEST) {}

Connection::~Connection() {
	(void) _clientFd;
	(void) _state;
	// /!\ do not close _clientFd, it should be managed by EventLopp
}
