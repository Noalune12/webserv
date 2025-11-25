#ifndef VALIDATOR_HPP
# define VALIDATOR_HPP

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
		// Config& _configData; plus on avance moins je sais ce qu'on met ici

	public:
		Validator();
		~Validator();

};

#endif
