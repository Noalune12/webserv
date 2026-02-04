#ifndef RESPONSESENDER_HPP
# define RESPONSESENDER_HPP

# include <sys/socket.h>

# include "Response.hpp"

class ResponseSender {

	private:
		std::vector<char>	prepareRawData(Response& resp);

	public:
		ResponseSender();
		~ResponseSender();

		ssize_t send(int clientFd, Response& resp);
};

#endif
