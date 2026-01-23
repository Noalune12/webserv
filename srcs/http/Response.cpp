#include <iostream>

#include "colors.hpp"
#include "Response.hpp"

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
}
