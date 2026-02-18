#include <arpa/inet.h>
#include <sstream>

#include "Connection.hpp"

Connection::Connection() : _ip(), _state(IDLE), _buffer(), _serverIP(), _serverPort(0), request(), cgi(), sendBuffer(), sendOffset(0)  {
	for (size_t i = 0; i < 5; ++i) {
		_timers[i] = time(NULL);
	}
}

Connection::Connection(int& clientFd, std::string& ip, std::vector<server>	servers, globalDir globalDir) : _ip(ip), _state(IDLE), _buffer(), _chunkBuffer(), _servers(servers), request(servers, globalDir), cgi(), sendBuffer(), sendOffset(0) {

	for (size_t i = 0; i < 5; ++i) {
		_timers[i] = time(NULL);
	}

	// Get the local address (server's ip:port that received this connection)
	struct sockaddr_storage local_addr;
	socklen_t addr_len = sizeof(local_addr);
	getsockname(clientFd, (struct sockaddr*)&local_addr, &addr_len);

	// Extract IP and Port
	if (local_addr.ss_family == AF_INET) {
		struct sockaddr_in* addr = (struct sockaddr_in*)&local_addr;

		unsigned char* bytes = (unsigned char*)&addr->sin_addr;
		std::stringstream ss;
		ss << (int)bytes[0] << "." << (int)bytes[1] << "." << (int)bytes[2] << "." << (int)bytes[3];
		_serverIP = ss.str();
		_serverPort = ntohs(addr->sin_port);
	}
}

Connection::~Connection() {
}

const std::string&	Connection::getIP(void) const {
	return (_ip);
}

ConnectionState	Connection::getState(void) const {
	return (_state);
}

bool	Connection::isTimedOut(int index) const {
	return (time(NULL) > _timers[index]);
}

void	Connection::setState(ConnectionState s) {
	_state = s;
}

void	Connection::startTimer(int index, time_t duration) {
	_timers[index] = time(NULL) + duration;
}

long	Connection::secondsToClosestTimeout() const {

	time_t	curr = time(NULL);
	int		active_idx = -1;

	switch (_state) {
		case IDLE:
			active_idx = 0;
			break ;
		case READING_HEADERS:
			active_idx = 1;
			break ;
		case READING_BODY:
			active_idx = 2;
			break ;
		case CGI_WRITING_BODY:
		case CGI_RUNNING:
			active_idx = 3;
			break ;
		case SENDING_RESPONSE:
			active_idx = 4;
			break ;
		default:
			return 5;
	}

	if (active_idx < 0)
		return (5);

	long	rem = static_cast<long>(_timers[active_idx] - curr);

	if (rem <= 0)
		return (1);

	return(rem);
}

void Connection::setBuffer(std::string request) {
	_buffer = request;
}

void Connection::setChunkBuffer(std::string request) {
	_chunkBuffer = request;
}

std::string Connection::getBuffer(void) const {
	return (_buffer);
}

std::string Connection::getChunkBuffer(void) const {
	return (_chunkBuffer);
}

void	Connection::clearChunkBuffer() {
	_chunkBuffer.clear();
}

void	Connection::clearBuffer() {
	_buffer.clear();
}

void	Connection::clearSendBuffer() {
    sendBuffer.clear();
    sendOffset = 0;
}


void Connection::parseRequest() {

	request.clearPreviousRequest();
	request.setServerInfo(_serverPort, _serverIP);
	request.checkRequestSem(_buffer);

	if (request.err == true)
		return ;

	request.checkRequestContent();
	_buffer.clear();
}
