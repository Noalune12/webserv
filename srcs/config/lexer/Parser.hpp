#ifndef PARSER_HPP
# define PARSER_HPP

# include <vector>

/*
 * Analyse syntaxique des tokens
 *
 * Responsabilités:
 * - Construit l'arbre de configuration à partir de la liste de tokens
 * - Identifie les blocs (global, server, location) et leur hiérarchie
 * - Associe les directives à leurs valeurs et à leur contexte
 * - Vérifie la syntaxe (accolades bien fermées, directives correctement terminées, etc.)
 * - Retourne une structure de données représentant la configuration parsée
 */
class Parser {

	private:
		// structure définie dans Tokenizer, je pense
		// size_t _currentIndex;

	public:
		Parser(/* const std::vector<Token>& tokens or not*/);
		~Parser();

};

#endif
