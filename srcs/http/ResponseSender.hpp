#ifndef RESPONSESENDER_HPP
# define RESPONSESENDER_HPP

# include <sys/socket.h>

# include "Response.hpp"

class ResponseSender {

	private:

	public:
		ResponseSender();
		~ResponseSender();

		std::vector<char>	prepareRawData(Response& resp);

		ssize_t send(int clientFd, Response& resp);
};

#endif
