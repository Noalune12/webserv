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

	public:

		~Logger();

		static void	enable(void);
		static void	disable(void);

		static void	test(void);
};

#endif
