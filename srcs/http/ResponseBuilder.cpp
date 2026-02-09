#include <fstream>
#include <sstream>

#include "colors.hpp"

#include "MimeTypes.hpp"
#include "ResponseBuilder.hpp"
#include "StatusCodes.hpp"
#include "Logger.hpp"

ResponseBuilder::ResponseBuilder() {}

ResponseBuilder::~ResponseBuilder() {}

Response	ResponseBuilder::buildFromRequest(const Request& req) {

	Response	resp;

	initializeResponse(resp, req);

	if (req._return) {
		setStatus(resp, req.status);
		resp._headers["Location"] = req._returnPath;
		resp._headers["Content-Type"] = "text/html";
		return (resp);
	}

	if (req.err && req.status == 405) {
		setAllow(resp, req);
	}

	if (req.err) {
		setStatus(resp, req.status);
		if (!req.htmlPage.empty()) {
			resp._body.assign(req.htmlPage.begin(), req.htmlPage.end());
		} else {
			setBodyFromError(resp, req.status, req);
		}
	}
	else if (!req.htmlPage.empty()) {
		setStatus(resp, 200);
		setBodyFromFile(resp, req);
	}
	else { // No content - return error
		setStatus(resp, req.err ? req.status : 500);
		setBodyFromError(resp, resp._statusCode, req);
	}
	return (resp);
}

Response	ResponseBuilder::buildFromCGI(const std::string& cgiOutput, const Request& req) {

	Response	resp;

	initializeResponse(resp, req);
	setStatus(resp, 200);

	parseCGIHeaders(cgiOutput, resp);
	parseCGIBody(cgiOutput, resp);
	setContentTypeFromCGI(resp);

	return (resp);
}

Response	ResponseBuilder::buildError(int statusCode, bool keepAlive) {

	Response	resp;

	setCommonHeaders(resp, keepAlive);
	setStatus(resp, statusCode);

	std::string errorPage = StatusCodes::generateDefaultErrorPage(statusCode);
	resp._body.assign(errorPage.begin(), errorPage.end());
	resp._headers["Content-Type"] = "text/html";

	return (resp);
}

void	ResponseBuilder::initializeResponse(Response& resp, const Request& req) {

	resp._statusCode = 200;
	resp._statusText = "OK";
	resp._headers.clear();
	resp._body.clear();
	resp._bytesSent = 0;

	setCommonHeaders(resp, req._keepAlive);
}

void	ResponseBuilder::setBodyFromFile(Response& resp, const Request& req) {
	resp._body.assign(req.htmlPage.begin(), req.htmlPage.end());
	setContentType(resp, req._trailing);
}


void	ResponseBuilder::setBodyFromError(Response& resp, int statusCode, const Request& req) {

	std::string customErrorPath = getCustomErrorPage(statusCode, req);

	if (!customErrorPath.empty()) {
		std::string customPage = loadErrorPageFile(customErrorPath, req);
		if (!customPage.empty()) {
			resp._body.assign(customPage.begin(), customPage.end());
			resp._headers["Content-Type"] = "text/html";
			Logger::debug("Loaded custom error page: " + customErrorPath);
			return ;
		}
	}

	// Fallback to default error page
	std::string errorPage = StatusCodes::generateDefaultErrorPage(statusCode);
	resp._body.assign(errorPage.begin(), errorPage.end());
	resp._headers["Content-Type"] = "text/html";
}

void	ResponseBuilder::parseCGIHeaders(const std::string& cgiOutput, Response& resp) {

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
					resp._statusCode = statusCode;

					std::string	statusText;
					std::getline(ss, statusText);
					if (!statusText.empty() && statusText[0] == ' ') {
						statusText = statusText.substr(1);
					}
					if (!statusText.empty()) {
						resp._statusText = statusText;
					} else {
						resp._statusText = StatusCodes::getReasonPhrase(statusCode);
					}
				}
			} else {
				resp._headers[name] = value;
			}
		}
	}
}

void	ResponseBuilder::parseCGIBody(const std::string& cgiOutput, Response& resp) {

	size_t		headerEnd = findHeaderEnd(cgiOutput);
	std::string	bodySection;

	if (headerEnd == std::string::npos) {
		bodySection = cgiOutput;
	} else {
		size_t bodyStart = (cgiOutput[headerEnd] == '\r') ? 4 : 2; // 4 = CRLF ("\r\n\r\n") and 2 = LF ("\n\n")
		bodySection = cgiOutput.substr(headerEnd + bodyStart);
	}

	resp._body.assign(bodySection.begin(), bodySection.end());
}

size_t	ResponseBuilder::findHeaderEnd(const std::string& cgiOutput) {

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

void	ResponseBuilder::setCommonHeaders(Response& resp, bool keepAlive) {
	resp._headers["Server"] = "webserv/1.0";
	resp._headers["Connection"] = keepAlive ? "keep-alive" : "close";
}

void	ResponseBuilder::setContentType(Response& resp, const std::string& extension) {
	std::string type = MimeTypes::getType(extension);
	resp._headers["Content-Type"] = type.empty() ? "text/html" : type;
}

void	ResponseBuilder::setContentTypeFromCGI(Response& resp) {
	if (resp._headers.find("Content-Type") == resp._headers.end()) {
		resp._headers["Content-Type"] = "text/html";
	}
}

void	ResponseBuilder::setStatus(Response& resp, int code) {
	resp._statusCode = code;
	resp._statusText = StatusCodes::getReasonPhrase(code);
}

void	ResponseBuilder::setAllow(Response& resp, const Request& req) {

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

	resp._headers["Allow"] = allowedMethods;
}

/* private methods */
std::string	ResponseBuilder::getCustomErrorPage(int statusCode, const Request& req) {

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

std::string	ResponseBuilder::loadErrorPageFile(const std::string& uriPath, const Request& req) {

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

	Logger::debug("Successfully loaded custom error page: " + fullPath);
	return (content.str());
}
