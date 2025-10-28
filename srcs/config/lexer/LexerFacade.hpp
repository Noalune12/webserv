#ifndef LEXERFACADE_HPP
# define LEXERFACADE_HPP

# include <string>

/*
 * LexerFacade - Sous-facade pour la phase de lexing et parsing
 *
 * Orchestre la transformation du fichier de configuration en structure de données:
 * 1. Tokenizer: transforme le texte (récupérer de FileReader) en tokens (mots-clés, valeurs, accolades, etc.)
 * 2. Parser: construit l'arbre de configuration é partir des tokens
 *
 * Gros doute sur certaines nécessité de délégation, c'est notre propre format de fichier de configuration alors est-ce qu'on a besoin d'aller aussi loin ? Besoin de ton point de vue
 *
 * Retourne une structure de données représentant la configuration parsée (non validée)
 */
class LexerFacade {

	private:
		std::string _fileContent; // full config récupérée depuis FileReader

	public:
		LexerFacade(const std::string& filePath);
		~LexerFacade();

		// Parse le fichier en appelant les classes Tokenizer -> Parser
		// Retourne la structure de données représentant le fichier de configuration
};

#endif
