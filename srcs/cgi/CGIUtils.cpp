#include <sys/wait.h>

#include "EventLoop.hpp"
#include "CGIExecutor.hpp"
#include "Logger.hpp"


void	CGIExecutor::cleanup(CGIContext& cgi) {

	cgi.closePipes();

	if (cgi.pid > 0) {
		int	status;
		waitpid(cgi.pid, &status, WNOHANG); // the flag makes it non blocking (for the eventloop)
		cgi.pid = -1;
	}
}


void	CGIExecutor::killProcess(pid_t pid) {
	if (pid > 0) {
		kill(pid, SIGKILL);
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
