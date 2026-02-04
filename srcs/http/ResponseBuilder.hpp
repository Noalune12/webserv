#ifndef RESPONSEBUILDER_HPP
# define RESPONSEBUILDER_HPP

# include <string>

# include "Request.hpp"
# include "Response.hpp"

class ResponseBuilder {

	private:

	public:
		ResponseBuilder();
		~ResponseBuilder();

		Response buildFromRequest(const Request& req);
		Response buildFromCGI(const std::string& cgiOutput, const Request& req);
		Response buildError(int statusCode, bool keepAlive);

		// Response initialization
		void initializeResponse(Response& resp, const Request& req);

		// Body handling
		void setBodyFromFile(Response& resp, const Request& req);
		void setBodyFromError(Response& resp, int statusCode);

		// CGI parsing
		void parseCGIHeaders(const std::string& cgiOutput, Response& resp);
		void parseCGIBody(const std::string& cgiOutput, Response& resp);
		size_t findHeaderEnd(const std::string& cgiOutput);

		// Header helpers
		void setCommonHeaders(Response& resp, bool keepAlive);
		void setContentType(Response& resp, const std::string& extension);
		void setContentTypeFromCGI(Response& resp);

		// Status helpers
		void setStatus(Response& resp, int code);
};

#endif
