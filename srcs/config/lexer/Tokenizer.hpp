#ifndef TOKENIZER_HPP
# define TOKENIZER_HPP

# include <string>
# include <vector>
# include <map>

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
		std::string _content;
		// std::vector<Token> _tokens; ?
		// std::map<Token, std::string>	tokens; ??
		// std::map<std::vector<Token>, std::string> ??? JE SAIS PAS QUOI METTRE ALED LOU-ANNE


	public:
		Tokenizer(const std::string& content);
		~Tokenizer();

		// Tokenize le contenu et retourne une liste de tokens
		// std::vector<Token> tokenize();
		// std::map<...>
		// je sais vraiment pas
};

#endif
