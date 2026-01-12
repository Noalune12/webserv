#include "Connection.hpp"

Connection::Connection() : _clientFd(-1), _ip(), _port(-1), _state(IDLE), _buffer(), _bufferLenght(-1), _keepAlive(true), err(false), status(0) {
	for (size_t i = 0; i < 5; ++i) {
		_timers[i] = time(NULL);
	}
}

Connection::Connection(int& clientFd, std::string& ip, int& port, std::vector<server>	servers) : _clientFd(clientFd), _ip(ip), _port(port), _state(IDLE), _buffer(), _bufferLenght(-1), _keepAlive(true), _servers(servers), err(false), status(0) {
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

#include <sstream>

void Connection::parseRequest() {
	reqParsing.checkRequestSem(_buffer);
	err = reqParsing.err;
	status = reqParsing.status;

	std::cout << err << " &&&&& " << status << std::endl;
	_buffer.clear();

	if (err == true)
		return ;
	// Host check
	std::map<std::string, std::string>::iterator it = reqParsing._headers.find("host");
	if (it == reqParsing._headers.end()) {
		err = true;
		status = 400;
	    std::cout << "error no host in headers " << std::endl;
		return ;
	}
	server s;
	location l;
	size_t sep = it->second.find(":");
	if (sep == 0 || sep == it->second.size() - 1) {
		err = true;
		status = 400;
	    std::cout << "error host has : at the start or the end" << std::endl;
		return ;
	}
	std::string name;
	int port;
	if (sep == std::string::npos) {
		name = it->second;
		port = 8080;
	} else {
		name = it->second.substr(0, sep);
		std::string temp = it->second.substr(sep + 1, it->second.size());
		std::stringstream ss(temp);
		ss >> port; // what if overflow
	}
	if (name == "localhost") {
		name = "127.0.0.1";
	}
	std::vector<server>::iterator itServer = _servers.begin();
	bool serverFound = false;
	for (; itServer != _servers.end(); itServer++) {
		std::vector<listenDirective>::iterator itListen = itServer->lis.begin();
		for (; itListen != itServer->lis.end(); itListen++) {
			if (itListen->port == port && (itListen->ip == name || itListen->ip == "0.0.0.0")) {
				// what about servername and virtual hosting
				std::cout << "server found with " << itListen->port << ", " << itListen->ip << std::endl;
				s = *itServer;
				serverFound = true;
				break;
			}
		}
		if (serverFound)
			break ;
	}

	if (itServer == _servers.end()) {
		err = true;
		status = 400;
	    std::cout << "error no compatible server found" << std::endl;
		return ;
	}

	// uri check
	std::vector<location>::iterator itLocation = s.loc.begin();
	for (; itLocation != s.loc.end(); itLocation++) {
		if (reqParsing._uri == itLocation->path) {
			std::cout << "location found at " << itLocation->path << std::endl;
			l = *itLocation;
			break;
		}
	}

	if (itLocation == s.loc.end()) {
		err = true;
		status = 404;
	    std::cout << "error no location found" << std::endl;
		return ;
	}

	// method check
	if (reqParsing._method == "GET" && l.methods.get == false) {
		err = true;
		status = 405;
	    std::cout << "error not allowed method" << std::endl;
		return ;
	}

	// body check + content max body size

	// get info
	

	// if (reqParsing._method == "GET" && )
	// check method is allowed and uri is defined
	// check if host is ok
	// check is body if content len is ok
}
