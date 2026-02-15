#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "Request.hpp"

class Response {

	private:
		int									_statusCode;
		std::string							_statusText;
		std::map<std::string, std::string>	_headers;
		std::vector<char>					_body;
		size_t								_bytesSent;
		std::string	getCustomErrorPage(int statusCode, const Request& req);
		std::string	loadErrorPageFile(const std::string& path, const Request& req);

		void	setLocation(const Request& req);
		void	setAllow(const Request& req);

		void	initializeResponse(const Request& req);

		void	setBodyFromFile(const Request& req);
		void	setBodyFromError(int statusCode, const Request& req);

		void	parseCGIHeaders(const std::string& cgiOutput);
		void	parseCGIBody(const std::string& cgiOutput);
		size_t	findHeaderEnd(const std::string& cgiOutput);

		void	setCommonHeaders(const Request& req);
		void	setContentType(const std::string& extension);
		void	setContentTypeFromCGI(void);

		void	setStatus(int code);

	public:
		Response();
		~Response();

		void	buildFromRequest(const Request& req);
		void	buildFromCGI(const std::string& cgiOutput, const Request& req);

		std::vector<char>	prepareRawData(void);

		int									getStatusCode(void) const;
		const std::string&					getStatusText(void) const;
		const std::vector<char>&			getBody(void) const;
		size_t								getBodySize(void) const;
		size_t								getBytesSent(void) const;

		void	debugPrintRequestData(const Request& req);
};

#endif
