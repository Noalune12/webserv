#ifndef CGIEXECUTOR_HPP
# define CGIEXECUTOR_HPP

# include <string>
# include <vector>

# include "EventLoop.hpp"
# include "Connection.hpp"
# include "Request.hpp"

class CGIExecutor {

	public:
		CGIExecutor();
		~CGIExecutor();

		bool	start(Connection& client, int clientFd, EventLoop& loop);
		void	cleanup(CGIContext& cgi);
		void	handlePipeEvent(Connection& client, int clientFd, int pipeFd, uint32_t events, EventLoop& loop);

	private:
		bool	createPipes(CGIContext& cgi);
		void	setupChildProcess(CGIContext& cgi, const Request& req, EventLoop& loop);

		std::vector<char*>	buildEnvironment(const Request& req);
		std::string			buildEnvVar(const std::string& name, const std::string& value);

		bool	writeBodyToCGI(CGIContext& cgi, const std::string& body);

		ssize_t	readFromPipe(int pipeFd, std::string& buffer);

		void	killProcess(pid_t pid);

		void	closeAllFds(EventLoop& loop);
};

#endif
