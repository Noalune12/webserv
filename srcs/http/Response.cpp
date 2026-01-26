#include <iostream>
#include <sstream>

#include "colors.hpp"
#include "Response.hpp"
#include "StatusCodes.hpp"

Response::Response() : _statusCode(200), _statusText("OK"), _headers(), _body(), _bytesSent(0) {}

Response::~Response() {}

void	Response::debugPrintRequestData(const Request& req) {
	std::cout << CYAN "\n========== RESPONSE DEBUG: Request Data ==========" RESET << std::endl;

	std::cout << YELLOW "Error state:" RESET << std::endl;
	std::cout << "  err    = " << (req.err ? "true" : "false") << std::endl;
	std::cout << "  status = " << req.status << std::endl;

	std::cout << YELLOW "\nRequest line:" RESET << std::endl;
	std::cout << "  _method = \"" << req._method << "\"" << std::endl;
	std::cout << "  _uri    = \"" << req._uri << "\"" << std::endl;

	std::cout << YELLOW "\nHeaders (" << req._headers.size() << "):" RESET << std::endl;
	std::map<std::string, std::string>::const_iterator it;
	for (it = req._headers.begin(); it != req._headers.end(); ++it) {
		std::cout << "  [" << it->first << "] = \"" << it->second << "\"" << std::endl;
	}

	std::cout << YELLOW "\nBody:" RESET << std::endl;
	if (req._body.empty()) {
		std::cout << "  (empty)" << std::endl;
	} else {
		std::cout << "  size = " << req._body.size() << " bytes" << std::endl;
		std::string preview = req._body;
		std::cout << "  FULL BODY BELOW\n" << preview << RESET;
	}

	std::cout << YELLOW "\nhtmlPage:" RESET << std::endl;
	if (req.htmlPage.empty()) {
		std::cout << "  (empty)" << std::endl;
	} else {
		std::cout << "  size = " << req.htmlPage.size() << " bytes" << std::endl;
	}
}


void	Response::prepare(const Request& req) {

	// a l'arrache pour l'instant
	_headers["Server"] = "webserv/1.0";
	_headers["Connection"] = "keep-alive";

	std::string bodyStr;

	bodyStr = StatusCodes::generateDefaultErrorPage(req.status);
	_body.assign(bodyStr.begin(), bodyStr.end());

	// Content-Length et Content-Type
	std::ostringstream lenOss;
	lenOss << _body.size();
	_headers["Content-Length"] = lenOss.str();
	_headers["Content-Type"] = "text/html";

	std::cout << bodyStr << std::endl;
}


std::vector<char>	Response::buildRaw(void) {

	std::ostringstream	oss;
	oss << "HTTP/1.1 " << _statusCode << " " << _statusText << "\r\n";

	std::map<std::string, std::string>::const_iterator it;
	for (it = _headers.begin(); it != _headers.end(); ++it) {
		oss << it->first << ": " << it->second << "\r\n";
	}
	oss << "\r\n"; // last CRLF (double \r\n)

	std::string	headerStr = oss.str();
	std::vector<char>	result;

	result.reserve(headerStr.size() + _body.size());

	// headers
	result.insert(result.end(), headerStr.begin(), headerStr.end());
	// body
	result.insert(result.end(), _body.begin(), _body.end());

	return (result);
}
