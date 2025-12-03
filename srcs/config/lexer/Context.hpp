#ifndef CONTEXT_HPP
# define CONTEXT_HPP

# include <vector>
# include <string>

# include "BindingsInfo.hpp"

struct Bindings {
	std::vector<std::pair<std::string, int> >	listenPairs;
	std::vector<std::string>					serverNames;
};

class Context {

	private:
		std::vector<std::pair<std::string, std::vector<std::string> > >	_directives;
		std::vector<Context>	_context;
		std::string				_name;


	public:
		std::vector<Bindings>	_bindingsInfo;
		Context(std::string name, std::string context);
		~Context();

		void	addDirective(std::string line);
		void	printContext() const;

		const std::string&														getName(void) const;
		const std::vector<Context>&												getContext(void) const;
		const std::vector<std::pair<std::string, std::vector<std::string> > >&	getDirectives(void) const;

		void	setBindingsInfo(const std::string& addr, const int& port);

		void	printBinding(void);
};

#endif
