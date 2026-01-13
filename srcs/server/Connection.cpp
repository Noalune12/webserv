#include "Connection.hpp"

Connection::Connection() : _clientFd(-1), _ip(), _port(-1), _state(IDLE), _buffer(), _bufferLenght(-1), _keepAlive(true) {
	for (size_t i = 0; i < 5; ++i) {
		_timers[i] = time(NULL);
	}
}

Connection::Connection(int& clientFd, std::string& ip, int& port, std::vector<server>	servers, globalDir globalDir) : _clientFd(clientFd), _ip(ip), _port(port), _state(IDLE), _buffer(), _bufferLenght(-1), _keepAlive(true), _servers(servers), _request(servers, globalDir) {
	for (size_t i = 0; i < 5; ++i) {
		_timers[i] = time(NULL);
	}
}

Connection::~Connection() {
	(void) _clientFd;
	(void) _ip;
	(void) _port;
	(void) _buffer;
	(void) _bufferLenght;
	(void) _keepAlive;
	// /!\ do not close _clientFd, it should be managed by EventLopp
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
	long	min_rem = 5;
	int		active_idx;

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
		case CGI_RUNNING:
			active_idx = 3;
			break ;
		case SENDING_RESPONSE:
			active_idx = 4;
			break ;
		default:
			return 5;
	}

	long	rem = _timers[active_idx] - curr;

	if (rem > 0 && rem < min_rem)
		return (rem);

	return(min_rem);
}

void Connection::setBuffer(std::string request) {
	_buffer = request;
}

std::string Connection::getBuffer(void) const {
	return _buffer;
}

void Connection::parseRequest() {
	_request.htmlPage.clear();
	_request.checkRequestSem(_buffer);
	if (_request.err == true)
		return ;
	_request.checkRequestContent();
	// err = _request.err;
	// status = _request.status;

	std::cout << _request.err << " &&&&& " << _request.status << std::endl;
	_buffer.clear();


}
