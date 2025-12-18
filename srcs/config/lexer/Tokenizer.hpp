#ifndef TOKENIZER_HPP
# define TOKENIZER_HPP

# include <string>
# include <vector>

# include "Context.hpp"
# include "define.h"

struct Token {
	/*
	 * des trucs ? un enum de type ? aled
	 */
};

/*
 * Tokenizer - Analyse lexicale du contenu du fichier
 *
 * Responsabilités:
 * - Transforme le texte brut en une liste de tokens
 * - Identifie les types de tokens: mots-clés (server, location, listen, etc.),
 *   valeurs, accolades ouvrantes/fermantes, points-virgules
 * - Ignore les commentaires (#) et les espaces/tabulations
 * - Gère les erreurs de syntaxe basiques (caractères invalides)
 */
class Tokenizer {

	private:
		std::vector<Context>								_context;
		PairVector	_globalDirectives;

	public:
		Tokenizer();
		Tokenizer(const std::string& fileContent);
		~Tokenizer();

		void addDirective(std::string line);
		void printContent() const;

		const PairVector&	getGlobalDirective(void) const;
		std::vector<Context>&	getVectorContext(void) ;
};

#endif
