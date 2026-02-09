#ifndef RESPONSEBUILDER_HPP
# define RESPONSEBUILDER_HPP

# include <string>

# include "Request.hpp"
# include "Response.hpp"

class ResponseBuilder {

	private:
		std::string	getCustomErrorPage(int statusCode, const Request& req);
		std::string	loadErrorPageFile(const std::string& path, const Request& req);

		void	setAllow(Response& resp, const Request& req);

	public:
		ResponseBuilder();
		~ResponseBuilder();

		Response	buildFromRequest(const Request& req);
		Response	buildFromCGI(const std::string& cgiOutput, const Request& req);
		Response	buildError(int statusCode, bool keepAlive);

		void	initializeResponse(Response& resp, const Request& req);

		void	setBodyFromFile(Response& resp, const Request& req);
		void	setBodyFromError(Response& resp, int statusCode, const Request& req);

		void	parseCGIHeaders(const std::string& cgiOutput, Response& resp);
		void	parseCGIBody(const std::string& cgiOutput, Response& resp);
		size_t	findHeaderEnd(const std::string& cgiOutput);

		void	setCommonHeaders(Response& resp, bool keepAlive);
		void	setContentType(Response& resp, const std::string& extension);
		void	setContentTypeFromCGI(Response& resp);

		void	setStatus(Response& resp, int code);
};

#endif
