# include <cstddef>
# include <ctime>
# include <iostream>
# include <sstream>
# include <string>

#include "colors.hpp"
#include "Logger.hpp"

LogLevel	Logger::_minLevel = DEBUG;
bool		Logger::_colored = true;
bool		Logger::_enabled = true;

Logger::Logger() {}

Logger::~Logger() {}

void	Logger::enable(void) {
	_enabled = true;
}

void	Logger::disable(void) {
	_enabled = false;
}

void	Logger::setColor(bool enable) {
	_colored = enable;
}

void	Logger::setMinLevel(LogLevel level) {
	_minLevel = level;
}

std::string	Logger::matchLevelToString(LogLevel level) {

	switch (level) {
		case DEBUG:
			return ("debug");
		case NOTICE:
			return ("notice");
		case WARN:
			return ("warn");
		case ERROR:
			return ("error");
		default:
			return ("unknown");
	}
}

const char*	Logger::matchLevelToColor(LogLevel level) {

	if (!_colored)
		return ("");

	switch (level) {
		case DEBUG:
			return (GREEN);
		case NOTICE:
			return (CYAN);
		case WARN:
			return (YELLOW);
		case ERROR:
			return (RED);
		default:
			return (RESET);
	}
}

std::string	Logger::formatTimestamp(void) {

	time_t		curr = time(NULL);
	struct tm*	t = localtime(&curr);
	char		buf[20];

	strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", t);

	return (std::string(buf));
}

void	Logger::log(LogLevel level, const std::string& m) {

	if (!_enabled || level < _minLevel)
		return ;

	const char*	color = matchLevelToColor(level);
	const char*	reset = _colored ? RESET : "";

	std::cout << color
		<< formatTimestamp() << " "
		<< "[" << matchLevelToString(level) << "] "
		<< m << reset << std::endl;
}


/**
 * @brief logging HTTP requests/response, must be called after send()
 *
 */
void	Logger::accessLog(const std::string& ip, const std::string& method, const std::string& uri, const std::string& version, int status, size_t bodySize) {

	if (!_enabled)
		return ;

	time_t		curr = time(NULL);
	struct tm*	t = localtime(&curr);
	char		buf[32];

	strftime(buf, sizeof(buf), "[%d/%b/%Y:%H:%M:%S +0000]", t);

	std::ostringstream oss;
	oss << ip << " - - " << buf << " "
		<< "\"" << method << " " << uri << " " << version << "\" "
		<< status << " " << bodySize;

	std::cout << oss.str() << std::endl;
}

void	Logger::debug(const std::string& m) {
	log(DEBUG, m);
}

void	Logger::notice(const std::string& m) {
	log(NOTICE, m);
}

void	Logger::warn(const std::string& m) {
	log(WARN, m);
}

void	Logger::error(const std::string& m) {
	log(ERROR, m);
}
