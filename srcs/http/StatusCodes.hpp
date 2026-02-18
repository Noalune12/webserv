#ifndef STATUSCODES_HPP
# define STATUSCODES_HPP

# include <string>

namespace StatusCodes {

	static const int OK							= 200;
	static const int CREATED					= 201; // post
	static const int NO_CONTENT					= 204; // delete

	static const int MOVED_PERMANENTLY			= 301;
	static const int FOUND						= 302;
	static const int SEE_OTHER          		= 303;
	static const int TEMP_REDIRECT				= 307;
	static const int PERM_REDIRECT				= 308;

	static const int BAD_request				= 400;
	static const int FORBIDDEN					= 403;
	static const int NOT_FOUND					= 404;
	static const int METHOD_NOT_ALLOWED			= 405; // pour HEAD specifiquement
	static const int REQUEST_TIMEOUT			= 408;
	static const int CONTENT_TOO_LARGE			= 413;

	static const int INTERNAL_SERVER_ERROR		= 500;
	static const int NOT_IMPLEMENTED			= 501; // pour TRACE etc -> HEAD goes to 405
	static const int HTTP_VERSION_NOT_SUPPORTED	= 505; // pour requete avec HTTP/1.4 par exemple

	std::string	getReasonPhrase(int code);

	bool	isRedirection(int code);
	bool	shouldForceClose(int code);
	bool	isError(int code);

	std::string	generateDefaultErrorPage(int code);
}

#endif
