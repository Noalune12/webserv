#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/epoll.h>

#include "colors.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"

CGIExecutor::CGIExecutor() {}

CGIExecutor::~CGIExecutor() {}

bool	CGIExecutor::start(Connection& client, int clientFd, EventLoop& loop) {

	CGIContext&	cgi = client._cgi;

	Logger::debug("Starting CGI: " + client._request.scriptPath);

	if (!createPipes(cgi)) {
		return (false);
	}

	if (!verifyCGIScript(client._request.scriptPath, client)) {
		cleanup(cgi, loop);
		return (false);
	}

	if (!verifyCGIPath(client._request.reqLocation->cgiPath.c_str(), client)) {
		cleanup(cgi, loop);
		return (false);
	}

	cgi.pid = fork();

	if (cgi.pid == -1) {
		cleanup(cgi, loop);
		Logger::error("fork() failed" + std::string(std::strerror(errno)));
		return (false);
	}

	if (cgi.pid == 0) {
		setupChildProcess(cgi, client, loop);
	}

	close(cgi.pipeIn[0]);	// Close read end of stdin
	cgi.pipeIn[0] = -1;
	close(cgi.pipeOut[1]);	// Close write end of stdout
	cgi.pipeOut[1] = -1;

	if (cgi.pipeIn[1] != -1) {
		if (fcntl(cgi.pipeIn[1], F_SETFL, O_NONBLOCK) < 0) {
			Logger::error("fnctl failed on CGI pipe write end: " + std::string(std::strerror(errno)));
			return (false);
		}
	}

	if (fcntl(cgi.pipeOut[0], F_SETFL, O_NONBLOCK) < 0) {
		Logger::error("fnctl failed on CGI pipe read end: " + std::string(std::strerror(errno)));
		return (false);
	}

	if (!client._request.fullBody.empty()) {
		cgi.inputBody = client._request.fullBody;
		cgi.inputOffset = 0;

		if (!loop.addToEpoll(cgi.pipeIn[1], EPOLLOUT)) {
			cleanup(cgi, loop);
			return (false);
		}

		loop.registerPipe(cgi.pipeIn[1], clientFd);
		// Logger::debug("CGI body to write, registered pipeIn for EPOLLOUT");
	} else {
		close(cgi.pipeIn[1]);
		cgi.pipeIn[1] = -1;

		if (!loop.addToEpoll(cgi.pipeOut[0], EPOLLIN)) {
			cleanup(cgi, loop);
			return (false);
		}
		loop.registerPipe(cgi.pipeOut[0], clientFd);
		// Logger::debug("No CGI body, registering pipeOut for EPOLLIN");
	}

	return (true);
}

void	CGIExecutor::handleCGIWriteEvent(Connection& client, int clientFd, int pipeFd, uint32_t events, EventLoop& loop) {

	CGIContext&	cgi = client._cgi;

	if (events & (EPOLLERR | EPOLLHUP)) {
		Logger::error("CGI stdin pipe error/hangup during body write");
		cleanup(cgi, loop);
		client._request.err = true;
		client._request.status = 502;
		client.setState(SENDING_RESPONSE);
		client.startTimer(4, DATA_MANAGEMENT_TIMEOUT);
		loop.modifyEpoll(clientFd, EPOLLIN | EPOLLOUT);
		return ;
	}

	if (events & EPOLLOUT) {
		const char*	data = cgi.inputBody.c_str() + cgi.inputOffset;
		size_t		remaining = cgi.inputBody.size() - cgi.inputOffset;

		ssize_t	written = write(pipeFd, data, remaining);

		if (written > 0) {
			cgi.inputOffset += static_cast<size_t>(written);
			client.startTimer(3, CGI_TIMEOUT); // reset CGI timeout

			if (cgi.inputOffset >= cgi.inputBody.size()) {
				transitionToReadingCGI(cgi, client, clientFd, loop);
			}
		} else if (written == -1) {
			Logger::error("CGI stdin write error.");
			cleanup(cgi, loop);
			client._request.err = true;
			client._request.status = 502;
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, DATA_MANAGEMENT_TIMEOUT);
			loop.modifyEpoll(clientFd, EPOLLIN | EPOLLOUT);
		}
	}
}

void	CGIExecutor::handlePipeEvent(Connection& client, int clientFd, int pipeFd, uint32_t events, EventLoop& loop) {

	CGIContext& cgi = client._cgi;

	if (events & EPOLLERR) {
		std::ostringstream oss;
		oss << "EPOLLER - fd[" << clientFd << "]";
		Logger::error(oss.str());
		cleanup(cgi, loop);
		client._request.err = true;
		client._request.status = 502;
		client.setState(SENDING_RESPONSE);
		client.startTimer(4, DATA_MANAGEMENT_TIMEOUT);
		loop.modifyEpoll(clientFd, EPOLLIN | EPOLLOUT);
		return ;
	}

	if (events & EPOLLIN) {
		ssize_t	bytesRead = readFromPipe(pipeFd, cgi.outputBuff);

		if (bytesRead > 0) {
			client.startTimer(3, CGI_TIMEOUT); // CGI_TIMEOUT -> same here, define ?
			// std::ostringstream	oss;
			// oss << bytesRead;
			// Logger::debug("CGI: read " + oss.str() + " bytes");
		}
		else if (bytesRead == 0) { // EOF - CGI finished
			// std::ostringstream oss;
			// oss << cgi.outputBuff.size();
			// Logger::debug("CGI finished (EOF), output size: " + oss.str());

			cleanup(cgi, loop);
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, DATA_MANAGEMENT_TIMEOUT); // CLIENT_TIMEOUT
			loop.modifyEpoll(clientFd,  EPOLLIN | EPOLLOUT);
			return ;
		}
		else if (bytesRead == -1) {
			Logger::error("CGI read error.");
			cleanup(cgi, loop);
			client._request.err = true;
			client._request.status = 502;
			client.setState(SENDING_RESPONSE);
			client.startTimer(4, DATA_MANAGEMENT_TIMEOUT); // CLIENT_TIMEOUT
			loop.modifyEpoll(clientFd,  EPOLLIN | EPOLLOUT);
			return ;
		}
	}

	if (events & (EPOLLHUP | EPOLLRDHUP)) {
		std::ostringstream oss;
		oss << cgi.outputBuff.size();
		Logger::warn("CGI pipe closed (EPOLLHUP), total output: " + oss.str());

		if (cgi.outputBuff.empty()) {
			client._request.err = true;
			client._request.status = 500;
		}

		cleanup(cgi, loop);
		client.setState(SENDING_RESPONSE);
		client.startTimer(4, DATA_MANAGEMENT_TIMEOUT);
		loop.modifyEpoll(clientFd,  EPOLLIN | EPOLLOUT);
		return;
	}
}

void	CGIExecutor::setupChildProcess(CGIContext& cgi, Connection& client, EventLoop& loop) {

	// close unused pipe
	close(cgi.pipeIn[1]);
	close(cgi.pipeOut[0]);

	// redirect stdin/stdout
	if (dup2(cgi.pipeIn[0], STDIN_FILENO) < 0 || dup2(cgi.pipeOut[1], STDOUT_FILENO) < 0) {
		close(cgi.pipeIn[0]);
		close(cgi.pipeOut[1]);
		throw std::runtime_error("dup2() failed: " + std::string(strerror(errno)));
	}

	// close original pipe fds
	close(cgi.pipeIn[0]);
	close(cgi.pipeOut[1]);

	closeAllFds(loop);

	std::string scriptDir = getDirectoryFromPath(client._request.scriptPath);  // "var/www/cgi-bin-py"
	std::string scriptFile = client._request.scriptPath.substr(scriptDir.length() + 1);  // "show-env.py"

	if (chdir(scriptDir.c_str()) != 0) {
		throw std::runtime_error("chdir() failed: " + std::string(strerror(errno)));
	}

	client._request.scriptPath = scriptFile;

	// Build environment
	std::vector<std::string> envStrings = buildEnvironmentStrings(client);

	std::vector<char*> env(envStrings.size() + 1);
	for (size_t i = 0; i < envStrings.size(); ++i) {
		env[i] = const_cast<char*>(envStrings[i].c_str());
	}
	env[envStrings.size()] = NULL;

	char* argv[] = {
		const_cast<char*>(client._request.reqLocation->cgiPath.c_str()),
		const_cast<char*>(client._request.scriptPath.c_str()),
		NULL
	};
	execve(argv[0], argv, &env[0]);
	throw std::runtime_error("CGI execve failed: " + std::string(strerror(errno)));
}

void	CGIExecutor::transitionToReadingCGI(CGIContext& cgi, Connection& client, int clientFd, EventLoop& loop) {

	loop.removeFromEpoll(cgi.pipeIn[1]);
	loop.unregisterPipe(cgi.pipeIn[1]);
	close(cgi.pipeIn[1]);
	cgi.pipeIn[1] = -1;

	cgi.inputBody.clear();
	cgi.inputOffset = 0;

	if (!loop.addToEpoll(cgi.pipeOut[0], EPOLLIN)) {
		cleanup(cgi, loop);
		client._request.err = true;
		client._request.status = 500;
		client.setState(SENDING_RESPONSE);
		client.startTimer(4, DATA_MANAGEMENT_TIMEOUT);
		loop.modifyEpoll(clientFd, EPOLLIN | EPOLLOUT);
		return ;
	}

	loop.registerPipe(cgi.pipeOut[0], clientFd);

	client.setState(CGI_RUNNING);
	client.startTimer(3, CGI_TIMEOUT); // CGI_TIMEOUT
}

ssize_t	CGIExecutor::readFromPipe(int pipeFd, std::string& buffer) {

	char tempBuffer[8192];
	ssize_t bytesRead = read(pipeFd, tempBuffer, sizeof(tempBuffer));

	if (bytesRead > 0) {
		buffer.append(tempBuffer, bytesRead);
	}

	return (bytesRead);
}
