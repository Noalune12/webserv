#ifndef VALIDATOR_HPP
# define VALIDATOR_HPP

# include "Config.hpp"

/*
 * Validation de la configuration
 *
 * Responsabilités:
 * - Vérifie que les directives sont valides dans leur contexte (ex: 'listen' uniquement dans server)
 * - Valide les valeurs des directives (ex: port entre 1-65535, taille avec unités correctes)
 * - Vérifie la cohérence globale (pas de ports en conflit, chemins valides, etc.)
 * - S'assure que les directives obligatoires sont présentes
 * - Renvoie des exceptions détaillées en cas d'erreur de validation et log ces erreurs dans ./var/log/error.log
 */
class Validator {

	private:
		Config&	_config;
		std::vector<std::pair<std::string, std::vector<std::string> > > _allowedInContext;

		void	initAllowedContext(void);
		void	initValidators(void);

		void	validateClientMaxBodySize(const std::vector<std::string>& values, const std::string& directive) const;
		void	keyNameCheck(const std::string& context) const;
		void	semicolonCheck(const std::vector<std::string>& v, const std::string& directive) const;

		void	logger(const std::string& error) const;


		void	validateGlobalDirective(void) const;
		// validateServerContexts()
		// validateLocationContexts()

		typedef void (Validator::*DirectiveValidator)(const std::vector<std::string>&) const;

		std::map<std::string, DirectiveValidator> _directiveValidators;


		void	directiveCheck(const std::string& directive, const std::vector<std::string>& values) const;


		void	validateErrorPage(const std::vector<std::string>& values) const;
		// void	validateClientMaxBodySize(const std::vector<std::string>& values) const; // adapt clientMaxBodySize()

		std::vector<std::vector<std::string> >	splitDirectiveGroups(const std::vector<std::string>& values) const;
		void	validateMinimumArgs(const std::vector<std::string>& group, size_t minArgs, const std::string& directive) const;


		// tmp - debug
		void	printGroups(const std::vector<std::vector<std::string> >& groups) const;

	public:

		Validator(Config& config);
		~Validator();

		void	printMap() const;

		// main function, calls every subverification
		void	validate(void);

};

#endif
