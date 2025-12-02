#ifndef CONTEXT_HPP
# define CONTEXT_HPP

# include <vector>
# include <string>

class Context {

	private:
		std::vector<std::pair<std::string, std::vector<std::string> > >	_directives;
		std::vector<Context>								_context;
		std::string											_name;

	public:
		Context(std::string name, std::string context);
		~Context();

		void addDirective(std::string line);
		void printContext() const;

};

#endif
