#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "Request.hpp"

class Response {

	private:

	public:
		Response();
		~Response();

		void	debugPrintRequestData(const Request& req);

		void	prepare(const Request& req);
		void	prepareCGI(const std::string& cgiOutput, const Request& req);

		std::vector<char>	prepareRawData(void);

		// data public for now
		int									_statusCode;
		std::string							_statusText;
		std::map<std::string, std::string>	_headers;
		std::vector<char>					_body;

		size_t								_bytesSent;
};


#endif
