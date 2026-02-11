#include <sstream>
#include <unistd.h>

#include "ResponseSender.hpp"
#include "Logger.hpp"

ResponseSender::ResponseSender() {}

ResponseSender::~ResponseSender() {}

ssize_t	ResponseSender::send(int clientFd, Response& resp) {

	std::vector<char>	rawData = prepareRawData(resp);

	ssize_t	bytesSent = ::send(clientFd, &rawData[0], rawData.size(), MSG_NOSIGNAL); // protect

	if (bytesSent > 0) {
		resp._bytesSent += bytesSent;
	}

	return (bytesSent);
}

std::vector<char>	ResponseSender::prepareRawData(Response& resp) {

	std::ostringstream oss;
	oss << "HTTP/1.1 " << resp._statusCode << " " << resp._statusText << "\r\n";

	std::ostringstream	lenOss;
	lenOss << resp._body.size();
	resp._headers["Content-Length"] = lenOss.str();

	std::map<std::string, std::string>::const_iterator	it;
	for (it = resp._headers.begin(); it != resp._headers.end(); ++it) {
		oss << it->first << ": " << it->second << "\r\n";
	}
	oss << "\r\n";

	std::string			headerStr = oss.str();
	std::vector<char>	res;

	res.reserve(headerStr.size() + resp._body.size());
	res.insert(res.end(), headerStr.begin(), headerStr.end());
	res.insert(res.end(), resp._body.begin(), resp._body.end());

	return (res);
}
