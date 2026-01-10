# include <string>
# include <iostream>
# include <sstream>
# include <ctime>

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

void	Logger::test(void) {
	std::cout << "YOOO" << std::endl;
}
