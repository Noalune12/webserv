#ifndef CONTEXT_HPP
# define CONTEXT_HPP

# include <vector>
# include <stdbool.h>

class Context {

	private:
		std::map<std::string, std::vector<std::string> >	_directives;
		std::vector<Context>								_context;
		std::string											_name;
		// bool												_close;



	public:

};

#endif
