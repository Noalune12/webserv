#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <cstddef>

# define ENABLE true
# define DISABLE false

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
		static void	accessLog(const std::string& ip, const std::string& method, const std::string& uri, const std::string& version, int status, size_t bodySize);

		// shortcuts
		static void	debug(const std::string& m);
		static void	notice(const std::string& m);
		static void	warn(const std::string& m);
		static void	error(const std::string& m);
};

#endif
