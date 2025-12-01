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

		/* utils global methods */
		void	logger(const std::string& error) const;
		void	keyNameCheck(const std::string& context) const;
		void	semicolonCheck(const std::vector<std::string>& v, const std::string& directive) const;
		void	validateMinimumArgs(const std::vector<std::string>& group, size_t minArgs, const std::string& directive) const;
		void	validateStrictArgsNb(const std::vector<std::string>& group, size_t exactNb, const std::string& directive) const;

		std::vector<std::vector<std::string> >	splitDirectiveGroups(const std::vector<std::string>& values) const;


		/* initialisation methods*/
		void	initAllowedContext(void);
		void	initValidators(void);


		/* specific directives checks*/
		void	validateClientMaxBodySize(const std::vector<std::string>& values) const;
		void	validateErrorPage(const std::vector<std::string>& values) const;

		void	validateServer(const std::vector<std::string>& group, const Context& context) const;


		/* subdivision of directives checks */
		void	validateGlobalDirective(void) const;
		void	validateServerContexts() const;
		// validateLocationContexts()


		/* member functions table pointer */
		typedef void (Validator::*DirectiveValidator)(const std::vector<std::string>&) const;
		std::map<std::string, DirectiveValidator> _directiveValidators;


		/* context parsing start */
		void	contextNameCheck(const Context& context) const;
		void	checkContextClosedProperly(const Context& context) const;

	public:

		Validator(Config& config);
		~Validator();

		/* debug methods */
		void	printMap() const; /* name has to be changed for printPairs or something */
		void	printGroups(const std::vector<std::vector<std::string> >& groups) const;
		void	printVector(const std::vector<std::string>& v) const;
		std::vector<std::string>	createVectorFromString(const std::string& str) const;


		// main method, calls every subverification
		void	validate(void);
};

#endif
