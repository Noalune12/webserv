#include <cstring>
#include "errno.h"
#include <signal.h>
#include <sstream>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "CGIExecutor.hpp"
#include "Logger.hpp"

CGIExecutor::CGIExecutor() {}

CGIExecutor::~CGIExecutor() {}

bool	CGIExecutor::start(Connection& client, int clientFd, EventLoop& loop) {

	CGIContext&	cgi = client._cgi;

	Logger::debug("Starting CGI: " + client._request._scriptPath);

	if (!createPipes(cgi)) {
		return (false);
	}

	cgi.pid = fork();

	if (cgi.pid == -1) {
		cleanup(cgi);
		Logger::error("fork() failed");
		return (false);
	}

	if (cgi.pid == 0) {
		setupChildProcess(cgi, client._request, loop);
	}

	close(cgi.pipeIn[0]);	// Close read end of stdin
	close(cgi.pipeOut[1]);	// Close write end of stdout

	if (!writeBodyToCGI(cgi, client._request._body)) {
		cleanup(cgi);
		return (false);
	}

	if (!loop.addToEpoll(cgi.pipeOut[0], EPOLLIN)) {
		cleanup(cgi);
		return (false);
	}

	loop.registerPipe(cgi.pipeOut[0], clientFd);

	return (true);
}

void	CGIExecutor::cleanup(CGIContext& cgi) {

	cgi.closePipes();

	if (cgi.pid > 0) {
		int	status;
		waitpid(cgi.pid, &status, WNOHANG); // the flag makes it non blocking (for the eventloop)
		cgi.pid = -1;
	}
}

void	CGIExecutor::handlePipeEvent(Connection& client, int clientFd, int pipeFd, uint32_t events, EventLoop& loop) {

	CGIContext& cgi = client._cgi;

	if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		Logger::debug("CGI pipe error/hangup");
		cleanup(cgi);
		client.setState(SENDING_RESPONSE);
		client.startTimer(4, 5); // CLIENT_TIMEOUT -> maybe change it for a define instead of a static member of EventLoop
		loop.modifyEpoll(clientFd, EPOLLOUT); // EPOLLIN | EPOLLOUT ? need to check later
		return ;
	}

	if (events & EPOLLIN) {
		ssize_t	bytesRead = readFromPipe(pipeFd, cgi.outputBuff);

		if (bytesRead > 0) {
			client.startTimer(3, 3); // CGI_TIMEOUT -> same here, define ?
			std::ostringstream	oss;
			oss << bytesRead;
			Logger::debug("CGI: read " + oss.str() + " bytes");
		}
		else if (bytesRead == 0) { // EOF - CGI finished
			std::ostringstream oss;
			oss << cgi.outputBuff.size();
			Logger::debug("CGI finished (EOF), output size: " + oss.str());

			cleanup(cgi);
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, 5); // CLIENT_TIMEOUT
			loop.modifyEpoll(clientFd, EPOLLOUT);
		}
		else if (errno != EAGAIN && errno != EWOULDBLOCK) {
			Logger::error("CGI read error: " + std::string(strerror(errno)));
			cleanup(cgi);
			client._request.err = true;
			client._request.status = 502;
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, 5); // CLIENT_TIMEOUT
			loop.modifyEpoll(clientFd, EPOLLOUT);
		}
	}
}

bool	CGIExecutor::createPipes(CGIContext& cgi) {

	if (pipe(cgi.pipeIn) == -1) {
		Logger::error("pipe(pipeIn) failed");
		return (false);
	}

	if (pipe(cgi.pipeOut) == -1) {
		close(cgi.pipeIn[0]);
		close(cgi.pipeIn[1]);
		Logger::error("pipe(pipeOut) failed");
		return (false);
	}

	return (true);
}


void	CGIExecutor::setupChildProcess(CGIContext& cgi, const Request& req, EventLoop& loop) {

	// close unused pipe
	close(cgi.pipeIn[1]);	// Close write end of stdin
	close(cgi.pipeOut[0]);	// Close read end of stdout

	// redirect stdin/stdout
	dup2(cgi.pipeIn[0], STDIN_FILENO);
	dup2(cgi.pipeOut[1], STDOUT_FILENO);

	// close original pipe fds
	close(cgi.pipeIn[0]);
	close(cgi.pipeOut[1]);

	// close all server file descriptors
	closeAllFds(loop);

	// Build environment
	std::vector<char*>	env = buildEnvironment(req);

	char* argv[] = {
		const_cast<char*>(req._reqLocation->cgiPath.c_str()),
		const_cast<char*>(req._scriptPath.c_str()),
		NULL
	};

	// std::cerr << RED "DEBUG\n" RESET << "cgiPath: " << req._reqLocation->cgiPath << "\nscript path: " << req._scriptPath << std::endl;

	execve(argv[0], argv, &env[0]);

	// if execve failed
	env.clear();
	std::cerr << "execve failed: " << strerror(errno) << std::endl;
	exit(EXIT_FAILURE);
}

std::vector<char*>	CGIExecutor::buildEnvironment(const Request& req) {

	std::vector<std::string>	envStrings;
	std::vector<char*>			envPointers;

	envStrings.push_back(buildEnvVar("REQUEST_METHOD", req._method));
	envStrings.push_back(buildEnvVar("QUERY_STRING", req._queryString));
	envStrings.push_back(buildEnvVar("SCRIPT_FILENAME", req._scriptPath));
	envStrings.push_back(buildEnvVar("SERVER_PROTOCOL", "HTTP/1.1"));
	envStrings.push_back(buildEnvVar("GATEWAY_INTERFACE", "CGI/1.1"));
	envStrings.push_back(buildEnvVar("REDIRECT_STATUS", "200"));

	std::map<std::string, std::string>::const_iterator it;
	it = req._headers.find("Content-Length");
	if (it != req._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", it->second));
	} else {
		envStrings.push_back(buildEnvVar("CONTENT_LENGTH", "0"));
	}

	it = req._headers.find("Content-Type");
	if (it != req._headers.end()) {
		envStrings.push_back(buildEnvVar("CONTENT_TYPE", it->second));
	}

	if (req._reqLocation) {
		// add SERVER_NAME, SERVER_PORT
	}

	for (size_t i = 0; i < envStrings.size(); ++i) {
		envPointers.push_back(const_cast<char*>(envStrings[i].c_str())); // Convertion to char* array for execve
	}
	envPointers.push_back(NULL); // null terminated env

	return (envPointers);
}

std::string	CGIExecutor::buildEnvVar(const std::string& name, const std::string& value) {
	return (name + "=" + value);
}

bool CGIExecutor::writeBodyToCGI(CGIContext& cgi, const std::string& body) {

	if (!body.empty()) {
		ssize_t written = write(cgi.pipeIn[1], body.c_str(), body.size()); // still not sure I'm allowed to write without going back to epoll
		if (written != static_cast<ssize_t>(body.size())) {
			Logger::error("Failed to write full body to CGI");
			return (false);
		}
	}

	close(cgi.pipeIn[1]); // sending EOF to the pipe by closing it
	cgi.pipeIn[1] = -1;

	Logger::debug("Sent body to CGI and closed stdin");
	return (true);
}

ssize_t	CGIExecutor::readFromPipe(int pipeFd, std::string& buffer) {

	char tempBuffer[8192];
	ssize_t bytesRead = read(pipeFd, tempBuffer, sizeof(tempBuffer)); // same same

	if (bytesRead > 0) {
		buffer.append(tempBuffer, bytesRead);
	}

	return (bytesRead);
}

void	CGIExecutor::killProcess(pid_t pid) {
	if (pid > 0) {
		kill(pid, SIGKILL);
	}
}

void	CGIExecutor::closeAllFds(EventLoop& loop) {

	std::vector<int>	clientFds = loop.getAllClientFds();
	for (size_t i = 0; i < clientFds.size(); ++i) {
		close(clientFds[i]);
	}

	loop.closeEpollFd();

	std::vector<int>	listenFds = loop.getListenSocketFds();
	for (size_t i = 0; i < listenFds.size(); ++i) {
		close(listenFds[i]);
	}
}
