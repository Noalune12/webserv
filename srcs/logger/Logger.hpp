#ifndef LOGGER_HPP
# define LOGGER_HPP

enum LogLevel {
	DEBUG,
	NOTICE,
	WARN,
	ERROR
};

class Logger {

	private:
		static LogLevel _minLevel;
		static bool		_colored;
		static bool		_enabled;

		Logger();

		static std::string	formatTimestamp(void);
		static std::string	matchLevelToString(LogLevel level);
		static const char*	matchLevelToColor(LogLevel level);

	public:

		~Logger();

		static void	enable(void);
		static void	disable(void);
		static void	setColor(bool enable);
		static void	setMinLevel(LogLevel level);

		static void	log(LogLevel level, const std::string& m);

		// shorcuts
		static void	debug(std::string m);
		static void	notice(std::string m);
		static void	warn(std::string m);
		static void	error(std::string m);
};

#endif
