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

	if (!verifyCGIScript(client._request._scriptPath, client)) {
		cleanup(cgi, loop);
		return (false);
	}

	if (!verifyCGIPath(client._request._reqLocation->cgiPath.c_str(), client)) {
		cleanup(cgi, loop);
		return (false);
	}

	cgi.pid = fork();

	if (cgi.pid == -1) {
		cleanup(cgi, loop); // add loop as parameter to removeFromEpoll properly
		Logger::error("fork() failed");
		return (false);
	}

	if (cgi.pid == 0) {
		setupChildProcess(cgi, client, loop);
	}

	close(cgi.pipeIn[0]);	// Close read end of stdin
	cgi.pipeIn[0] = -1;
	close(cgi.pipeOut[1]);	// Close write end of stdout
	cgi.pipeOut[1] = -1;

	if (!writeBodyToCGI(cgi, client._request._body)) {
		cleanup(cgi, loop);
		return (false);
	}

	if (!loop.addToEpoll(cgi.pipeOut[0], EPOLLIN)) {
		cleanup(cgi, loop);
		return (false);
	}

	loop.registerPipe(cgi.pipeOut[0], clientFd);

	return (true);
}


void	CGIExecutor::handlePipeEvent(Connection& client, int clientFd, int pipeFd, uint32_t events, EventLoop& loop) {

	CGIContext& cgi = client._cgi;

	if (events & EPOLLERR) {
		std::cerr << RED "EPOLLERR - fd[" << clientFd << "]" RESET << std::endl;
		Logger::error("CGI pipe error");
		cleanup(cgi, loop);
		client._request.err = true;
		client._request.status = 502;
		client.setState(SENDING_RESPONSE);
		client.startTimer(4, 5);
		loop.modifyEpoll(clientFd, EPOLLOUT);
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

			cleanup(cgi, loop);
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, 5); // CLIENT_TIMEOUT
			loop.modifyEpoll(clientFd, EPOLLOUT);
			return ;
		}
		else if (errno != EAGAIN && errno != EWOULDBLOCK) {
			Logger::error("CGI read error: " + std::string(strerror(errno)));
			cleanup(cgi, loop);
			client._request.err = true;
			client._request.status = 502;
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, 5); // CLIENT_TIMEOUT
			loop.modifyEpoll(clientFd, EPOLLOUT);
			return ;
		}
	}

	if (events & (EPOLLHUP | EPOLLRDHUP)) {
		if (events & EPOLLHUP) {
			std::cerr << RED "EPOLLHUP - fd[" << clientFd << "] (which is the expected behavior)" RESET << std::endl;
		} else if (events & EPOLLRDHUP) {
			std::cerr << RED "EPOLLRDHUP - fd[" << clientFd << "]" RESET << std::endl;
		}
		std::ostringstream oss;
		oss << cgi.outputBuff.size();
		Logger::warn("CGI pipe closed (EPOLLHUP), total output: " + oss.str());

		cleanup(cgi, loop);
		client.setState(SENDING_RESPONSE);
		client.startTimer(4, 5);
		loop.modifyEpoll(clientFd, EPOLLOUT);
		return;
	}
}

void	CGIExecutor::setupChildProcess(CGIContext& cgi, Connection& client, EventLoop& loop) {

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

	std::string scriptDir = getDirectoryFromPath(client._request._scriptPath);  // "var/www/cgi-bin-py"
	std::string scriptFile = client._request._scriptPath.substr(scriptDir.length() + 1);  // "show-env.py"

	// chdir to cgi binary
	if (chdir(scriptDir.c_str()) != 0) {
		std::cerr << "chdir() failed: " << strerror(errno) << std::endl;
		// leak fd, need to close first
		exit(EXIT_FAILURE);
	}

	client._request._scriptPath = scriptFile;

	// Build environment
	std::vector<std::string> envStrings = buildEnvironmentStrings(client);

	std::vector<char*> env(envStrings.size() + 1);
	for (size_t i = 0; i < envStrings.size(); ++i) {
		env[i] = const_cast<char*>(envStrings[i].c_str());
	}
	env[envStrings.size()] = NULL;

	char* argv[] = {
		const_cast<char*>(client._request._reqLocation->cgiPath.c_str()),
		const_cast<char*>(client._request._scriptPath.c_str()),
		NULL
	};
	execve(argv[0], argv, &env[0]);

	// if execve failed
	std::cerr << "execve failed: " << strerror(errno) << std::endl;
	exit(EXIT_FAILURE);
}

bool CGIExecutor::writeBodyToCGI(CGIContext& cgi, const std::string& body) {

	if (!body.empty()) {
		ssize_t written = write(cgi.pipeIn[1], body.c_str(), body.size()); // still not sure I'm allowed to write without going back to epoll
		if (written != static_cast<ssize_t>(body.size())) {
			Logger::error("Failed to write full body to CGI");
			return (false);
		}
	}

	close(cgi.pipeIn[1]); // sending EOF to the pipe by closing it -> sends a signal that the request body is complete
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
