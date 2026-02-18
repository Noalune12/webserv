#ifndef TOKENIZER_HPP
# define TOKENIZER_HPP

# include "Context.hpp"

class Tokenizer {

	private:
		std::vector<Context>								_context;
		PairVector	_globalDirectives;
		std::string _fileContent;

		void addDirective(std::string line);

	public:
		Tokenizer();
		~Tokenizer();

		void tokenize(const std::string& fileContent);
		void printContent() const;
		const PairVector&	getGlobalDirective(void) const;
		std::vector<Context>&	getVectorContext(void) ;
};

#endif
