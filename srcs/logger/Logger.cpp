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
			return (GRAY);
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


void	Logger::test(void) {
	std::cout << GRAY "SOMETHING" RESET << std::endl;
}
