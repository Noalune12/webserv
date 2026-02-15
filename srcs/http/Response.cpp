#include <fstream>
#include <iostream>
#include <sstream>

#include "colors.hpp"
#include "Logger.hpp"
#include "MimeTypes.hpp"
#include "Response.hpp"
#include "StatusCodes.hpp"

Response::Response() : _statusCode(200), _statusText("OK"), _headers(), _body(), _bytesSent(0) {}

Response::~Response() {}

void	Response::buildFromRequest(const Request& req) {

	initializeResponse(req);

	if (req._return) {
		setStatus(req.status);
		_headers["Location"] = req._returnPath;
		_headers["Content-Type"] = "text/html";
		return ;
	}

	if (req.err && req.status == 405) {
		setAllow(req);
	}

	if (req.status == 201 || (req._method == "POST" && req.status == 200)) {
		setLocation(req);
		setStatus(req.status);
		return ;
	}

	if (req.err) {
		setStatus(req.status);
		if (!req.htmlPage.empty()) {
			_body.assign(req.htmlPage.begin(), req.htmlPage.end());
		} else {
			setBodyFromError(req.status, req);
		}
	} else {
		setStatus(req.status);
		setBodyFromFile(req);
	}
}

void	Response::buildFromCGI(const std::string& cgiOutput, const Request& req) {

	initializeResponse(req);
	setStatus(200);

	parseCGIHeaders(cgiOutput);
	parseCGIBody(cgiOutput);
	setContentTypeFromCGI();
}

std::vector<char>	Response::prepareRawData(void) {

	std::ostringstream	oss;
	oss << "HTTP/1.1 " << _statusCode << " " << _statusText << "\r\n";

	std::ostringstream	lenOss;
	lenOss << _body.size();
	_headers["Content-Length"] = lenOss.str();

	std::map<std::string, std::string>::const_iterator	it;
	for (it = _headers.begin(); it != _headers.end(); ++it) {
		oss << it->first << ": " << it->second << "\r\n";
	}
	oss << "\r\n";

	std::string			headerStr = oss.str();
	std::vector<char>	res;

	res.reserve(headerStr.size() + _body.size());
	res.insert(res.end(), headerStr.begin(), headerStr.end());
	res.insert(res.end(), _body.begin(), _body.end());

	return (res);
}

int	Response::getStatusCode(void) const {
	return (_statusCode);
}

const std::string&	Response::getStatusText(void) const {
	return (_statusText);
}

const std::vector<char>&	Response::getBody(void) const {
	return (_body);
}

size_t	Response::getBodySize(void) const {
	return (_body.size());
}

size_t	Response::getBytesSent(void) const {
	return (_bytesSent);
}

void	Response::debugPrintRequestData(const Request& req) {
	std::cout << CYAN "\n========== RESPONSE DEBUG ==========" RESET << std::endl;

	std::cout << YELLOW "Error state:" RESET << std::endl;
	std::cout << "  err    = " << (req.err ? "true" : "false") << std::endl;
	std::cout << "  status = " << req.status << std::endl;

	std::cout << YELLOW "Request line:" RESET << std::endl;
	std::cout << "  method = \"" << req._method << "\"" << std::endl;
	std::cout << "  uri    = \"" << req._uri << "\"" << std::endl;
	std::cout << "  trailing    = \"" << req._trailing << "\"" << std::endl;

	std::cout << YELLOW "htmlPage:" RESET << std::endl;
	if (req.htmlPage.empty()) {
		std::cout << "  (empty)" << std::endl;
	} else {
		std::cout << "  size = " << req.htmlPage.size() << " bytes" << std::endl;
	}

	std::cout << YELLOW "Location:" RESET << std::endl;
	if (req._reqLocation != NULL) {
		std::cout << "  path  = \"" << req._reqLocation->path << "\"" << std::endl;
		std::cout << "  root  = \"" << req._reqLocation->root << "\"" << std::endl;
		std::cout << "  alias = \"" << req._reqLocation->alias << "\"" << std::endl;
	} else {
		std::cout << "  (no matching location)" << std::endl;
	}

	std::cout << YELLOW "Query:" RESET << std::endl;
	std::cout << "   query = \"" << req._queryString << "\"" << std::endl;

	std::cout << CYAN "====================================\n" RESET << std::endl;
}

void	Response::initializeResponse(const Request& req) {

	_statusCode = 200;
	_statusText = "OK";
	_headers.clear();
	_body.clear();
	_bytesSent = 0;

	setCommonHeaders(req);
}

void	Response::setStatus(int code) {
	_statusCode = code;
	_statusText = StatusCodes::getReasonPhrase(code);
}

void	Response::setBodyFromFile(const Request& req) {
	_body.assign(req.htmlPage.begin(), req.htmlPage.end());
	setContentType(req._trailing);
}

void	Response::setBodyFromError(int statusCode, const Request& req) {

	std::string customErrorPath = getCustomErrorPage(statusCode, req);

	if (!customErrorPath.empty()) {
		std::string customPage = loadErrorPageFile(customErrorPath, req);
		if (!customPage.empty()) {
			_body.assign(customPage.begin(), customPage.end());
			_headers["Content-Type"] = "text/html";
			Logger::debug("Loaded custom error page: " + customErrorPath);
			return ;
		}
	}

	std::string errorPage = StatusCodes::generateDefaultErrorPage(statusCode);
	_body.assign(errorPage.begin(), errorPage.end());
	_headers["Content-Type"] = "text/html";
}

void	Response::parseCGIHeaders(const std::string& cgiOutput) {

	size_t	headerEnd = findHeaderEnd(cgiOutput);

	if (headerEnd == std::string::npos) {
		return ; // no headers, only body
	}

	std::string			headerSection = cgiOutput.substr(0, headerEnd);
	std::istringstream	headerStream(headerSection);
	std::string			line;

	while (std::getline(headerStream, line)) {
		// remove \r
		if (!line.empty() && line[line.size() - 1] == '\r') {
			line.erase(line.size() - 1);
		}

		size_t	colonPos = line.find(':');

		if (colonPos != std::string::npos) {
			std::string name = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);

			size_t start = value.find_first_not_of(" \t");
			if (start != std::string::npos) {
				value = value.substr(start);
			}

			if (name == "Status") {
				std::istringstream	ss(value);
				int					statusCode;
				ss >> statusCode;

				if (ss) {
					setStatus(statusCode);
				}
			} else {
				_headers[name] = value;
			}
		}
	}
}

void	Response::parseCGIBody(const std::string& cgiOutput) {

	size_t		headerEnd = findHeaderEnd(cgiOutput);
	std::string	bodySection;

	if (headerEnd == std::string::npos) {
		bodySection = cgiOutput;
	} else {
		size_t bodyStart = (cgiOutput[headerEnd] == '\r') ? 4 : 2; // 4 = CRLF ("\r\n\r\n") and 2 = LF ("\n\n")
		bodySection = cgiOutput.substr(headerEnd + bodyStart);
	}

	_body.assign(bodySection.begin(), bodySection.end());
}

size_t	Response::findHeaderEnd(const std::string& cgiOutput) {

	size_t	pos = cgiOutput.find("\r\n\r\n");
	if (pos != std::string::npos) {
		return (pos);
	}

	pos = cgiOutput.find("\n\n");
	if (pos != std::string::npos) {
		return (pos);
	}

	return (std::string::npos);
}

void	Response::setCommonHeaders(const Request& req) {
	_headers["Server"] = "webserv/1.0";
	if (req._keepAlive && !req.err) {
		_headers["Connection"] = "keep-alive";
	} else {
		_headers["Connection"] = "close";
	}
}

void	Response::setContentType(const std::string& extension) {
	std::string type = MimeTypes::getType(extension);
	_headers["Content-Type"] = type.empty() ? "text/html" : type;
}

void	Response::setContentTypeFromCGI(void) {
	if (_headers.find("Content-Type") == _headers.end()) {
		_headers["Content-Type"] = "text/html";
	}
}

void	Response::setLocation(const Request& req) {

	if (req._uplaodFiles.empty())
		return ;
	_headers["Location"] = req._uplaodFiles[0].location + req._uplaodFiles[0].filename;
}

void	Response::setAllow(const Request& req) {

	if (req._reqLocation == NULL)
		return ;

	std::string	allowedMethods = "";

	if (req._reqLocation->methods.get)
		allowedMethods += "GET";

	if (req._reqLocation->methods.post) {
		if (!allowedMethods.empty())
			allowedMethods += ", ";
		allowedMethods += "POST";
	}

	if (req._reqLocation->methods.del) {
		if (!allowedMethods.empty())
			allowedMethods += ", ";
		allowedMethods += "DELETE";
	}

	_headers["Allow"] = allowedMethods;
}

std::string	Response::getCustomErrorPage(int statusCode, const Request& req) {

	if (req._reqLocation == NULL) {
		return ("");
	}

	std::map<int, std::string>::const_iterator	it;
	it = req._reqLocation->errPage.find(statusCode);

	if (it != req._reqLocation->errPage.end()) {
		return (it->second);
	}

	return ("");
}

std::string	Response::loadErrorPageFile(const std::string& uriPath, const Request& req) {

	if (req._reqLocation == NULL) {
		return ("");
	}

	std::string	fullPath;

	if (!req._reqLocation->alias.empty()) {
		fullPath = req._reqLocation->alias + uriPath;
	} else {
		fullPath = req._reqLocation->root + uriPath;
	}

	Logger::debug("Trying to load error page: " + fullPath);

	std::ifstream file(fullPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		Logger::warn("Could not open custom error page: " + fullPath);
		return ("");
	}

	std::ostringstream content;
	content << file.rdbuf();
	file.close();

	if (content.str().empty()) {
		Logger::warn("Error page file was empty or read failed: " + fullPath);
		return ("");
	}

	Logger::debug("Successfully loaded custom error page: " + fullPath);
	return (content.str());
}
