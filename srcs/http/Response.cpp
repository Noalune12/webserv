#include <iostream>
#include <sstream>

#include "colors.hpp"
#include "Response.hpp"
#include "StatusCodes.hpp"
#include "MimeTypes.hpp"

Response::Response() : _statusCode(200), _statusText("OK"), _headers(), _body(), _bytesSent(0) {}

Response::~Response() {}

void	Response::debugPrintRequestData(const Request& req) {
	std::cout << CYAN "\n========== RESPONSE DEBUG ==========" RESET << std::endl;

	std::cout << YELLOW "Error state:" RESET << std::endl;
	std::cout << "  err    = " << (req.err ? "true" : "false") << std::endl;
	std::cout << "  status = " << req.status << std::endl;

	std::cout << YELLOW "Request line:" RESET << std::endl;
	std::cout << "  method = \"" << req._method << "\"" << std::endl;
	std::cout << "  uri    = \"" << req._uri << "\"" << std::endl;

	std::cout << YELLOW "htmlPage:" RESET << std::endl;
	if (req.htmlPage.empty()) {
		std::cout << "  (empty)" << std::endl;
	} else {
		std::cout << "  size = " << req.htmlPage.size() << " bytes" << std::endl;
	}

	std::cout << YELLOW "Location:" RESET << std::endl;
	std::cout << "  path  = \"" << req._reqLocation->path << "\"" << std::endl;
	std::cout << "  root  = \"" << req._reqLocation->root << "\"" << std::endl;
	std::cout << "  alias = \"" << req._reqLocation->alias << "\"" << std::endl;

	std::cout << YELLOW "Query:" RESET << std::endl;
	std::cout << "   query = \"" << req._queryString << "\"" << std::endl;

	std::cout << CYAN "====================================\n" RESET << std::endl;
}

void	Response::prepare(const Request& req) {

	_statusCode = 200;
	_statusText = "OK";
	_headers.clear();
	_body.clear();
	_bytesSent = 0;

	_headers["Server"] = "webserv/1.0";
	_headers["Connection"] = req._keepAlive ? "keep-alive" : "close";

	if (req.err) {

		_statusCode = req.status;
		_statusText = StatusCodes::getReasonPhrase(req.status);

		if (!req.htmlPage.empty()) {
			_body.assign(req.htmlPage.begin(), req.htmlPage.end());
		} else {
			std::string errorPage = StatusCodes::generateDefaultErrorPage(req.status);
			_body.assign(errorPage.begin(), errorPage.end());
		}
		// std::string type = MimeTypes::getType(req._uri);
		// _headers["Content-Type"] = type;
		_headers["Content-Type"] = "text/html"; // a changer en appelant les functions de MimeTypes

		return ;
	}

	if (!req.htmlPage.empty()) {

		_statusCode = 200;
		_statusText = "OK";
		_body.assign(req.htmlPage.begin(), req.htmlPage.end());

		std::string type = MimeTypes::getType(req._uri);
		std::cout << RED << req._uri << RESET << std::endl;
		std::cout << RED << type << RESET << std::endl;

		// _headers["Content-Type"] = type;
		_headers["Content-Type"] = "text/html"; // a changer en appelant les functions de MimeTypes
		return ;
	}

	// chunked ?
	_statusCode = 500;
	_statusText = "Internal Server Error";
	std::string errorPage = StatusCodes::generateDefaultErrorPage(500);
	_body.assign(errorPage.begin(), errorPage.end());
	_headers["Content-Type"] = "text/html";
}


std::vector<char>	Response::buildRaw(void) {

	std::ostringstream	oss;
	oss << "HTTP/1.1 " << _statusCode << " " << _statusText << "\r\n";

	std::ostringstream lenOss;
	lenOss << _body.size();
	_headers["Content-Length"] = lenOss.str();

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
