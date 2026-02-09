#include "StatusCodes.hpp"

#include <sstream>

namespace StatusCodes {

std::string	getReasonPhrase(int code) {

	switch (code) {
		case 200:
			return "OK";
		case 201:
			return "Created";

		case 301:
			return "Moved Permanently";
		case 302:
			return "Found";
		case 303:
			return "See Other";
		case 307:
			return "Temporary Redirect";
		case 308:
			return "Permanent Redirect";

		case 400:
			return "Bad Request";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 408:
			return "Request Timeout";
		case 413:
			return "Content Too Large";

		case 500:
			return "Internal Server Error";
		case 501:
			return "Not Implemented";
		case 504:
			return "Gateway Timeout";
		case 505:
			return "HTTP Version Not Supported";

		default:
			return "Unknown";
	}
}

bool	isRedirection(int code) {
	return (code >= 300 && code < 400);
}

bool	shouldForceClose(int code) {
	return (code == 400 || code == 408 || code == 413 || code == 500);
}

bool	isError(int code) {
	return (code >= 400);
}

std::string	generateDefaultErrorPage(int code) {

	std::string reason = getReasonPhrase(code);

	std::ostringstream oss;
	oss << code;
	std::string codeStr = oss.str();

	std::string html =
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"    <meta charset=\"UTF-8\">\n"
		"    <title>" + codeStr + " " + reason + "</title>\n"
		"</head>\n"
		"<body>\n"
		"    <h1>" + codeStr + "</h1>\n"
		"    <p>" + reason + "</p>\n"
		"</body>\n"
		"</html>\n";

	return (html);
}

}
