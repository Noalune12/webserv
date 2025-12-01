#ifndef CONTEXT_HPP
# define CONTEXT_HPP

# include <vector>
# include <map>
# include <string>

class Context {

	private:
		std::vector<std::pair<std::string, std::vector<std::string> > >	_directives;
		std::vector<Context>								_context;
		std::string											_name;
		// bool												_close;

	public:
		// Context(std::vector<std::string> context);
		Context(std::string name, std::string context);
		~Context();

		// bool isOnlyWSpace(std::string line) const;
		void addDirective(std::string line);
		// void printMap() const;
		void printContext() const;

};

#endif
