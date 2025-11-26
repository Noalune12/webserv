#ifndef VALIDATOR_HPP
# define VALIDATOR_HPP

# include "Config.hpp"

// # include "rules.h" // header full of define ?

/*
 * Validation de la configuration
 *
 * Responsabilités:
 * - Vérifie que les directives sont valides dans leur contexte (ex: 'listen' uniquement dans server)
 * - Valide les valeurs des directives (ex: port entre 1-65535, taille avec unités correctes)
 * - Vérifie la cohérence globale (pas de ports en conflit, chemins valides, etc.)
 * - S'assure que les directives obligatoires sont présentes
 * - Renvoie des exceptions détaillées en cas d'erreur de validation (retour d'erreur hyper précis)
 */
class Validator {

	private:
		// Config& _configData; // plus on avance moins je sais ce qu'on met ici


	public:
		std::map<std::string, std::vector<std::string> >	_globalDirectives;

		Validator();
		~Validator();

		void	printMap() const;

		void	clientMaxBodySize(void) const;



		void	keyNameCheck(void) const;
		void	semicolonCheck(const std::vector<std::string>& v) const;
};

#endif
