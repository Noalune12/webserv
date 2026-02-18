#ifndef CONTEXT_HPP
# define CONTEXT_HPP

# include "define.h"

struct Bindings {
	std::vector<std::pair<std::string, int> >	listenPairs;
	std::vector<std::string>					serverNames;

	Bindings() : listenPairs(), serverNames() {}

	bool	checkDuplicateListenPair(const std::string& addr, int port) const {
		for (size_t i = 0; i < listenPairs.size(); ++i) {
			if (listenPairs[i].first == addr && listenPairs[i].second == port)
				return (true);
		}
		return (false);
	}

	bool	checkDuplicateServerName(const std::string& name) const {
		for (size_t i = 0; i < serverNames.size(); ++i) {
			if (serverNames[i] == name)
				return (true);
		}
		return (false);
	}
};

class Context {

	private:
		PairVector				_directives;
		std::vector<Context>	_context;
		std::string				_name;
		Bindings				_bindingsInfo;
		bool					_isIgnored;

		void	addDirective(std::string line);

	public:
		Context(std::string name, std::string context);
		~Context();

		void	printContext() const;

		const std::string&			getName(void) const;
		const std::vector<Context>&	getContext(void) const;
		std::vector<Context>&		getContext(void);
		const PairVector&			getDirectives(void) const;

		const Bindings&				getBinding(void) const;
		Bindings&					getBinding(void);
		bool						getIsIgnored(void) const;

		void	setIsIgnored(void);

		void	addListenPair(const std::string& addr, const int& port, const std::string& filePath);
		void	addServerName(const std::string& name, const std::string& filePath);
};

#endif
