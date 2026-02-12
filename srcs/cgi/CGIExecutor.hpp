#ifndef CGIEXECUTOR_HPP
# define CGIEXECUTOR_HPP

# include <stdint.h>

# include "Connection.hpp"

class EventLoop;
class Connection;

class CGIExecutor {

	public:
		CGIExecutor();
		~CGIExecutor();

		bool	start(Connection& client, int clientFd, EventLoop& loop);
		void	cleanup(CGIContext& cgi, EventLoop& loop);
		void	handlePipeEvent(Connection& client, int clientFd, int pipeFd, uint32_t events, EventLoop& loop);
		void	handleCGIWriteEvent(Connection& client, int clientFd, int pipeFd, uint32_t events, EventLoop& loop);

	private:
		bool	createPipes(CGIContext& cgi);
		void	setupChildProcess(CGIContext& cgi, Connection& client, EventLoop& loop);

		std::vector<std::string>	buildEnvironmentStrings(const Connection& conn);
		std::string					buildEnvVar(const std::string& name, const std::string& value);

		void	transitionToReadingCGI(CGIContext& cgi, Connection& client, int clientFd, EventLoop& loop);

		ssize_t	readFromPipe(int pipeFd, std::string& buffer);

		void	killProcess(pid_t pid);

		void	closeAllFds(EventLoop& loop);

		std::string	getDirectoryFromPath(const std::string& path);

		bool	verifyCGIPath(const std::string& path, Connection& client);
		bool	verifyCGIScript(const std::string& scriptPath, Connection& client);
};

#endif
