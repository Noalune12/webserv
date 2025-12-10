#ifndef VALIDATOR_HPP
# define VALIDATOR_HPP

# include <map>

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
		Config&		_config;
		Context*	_currentContext; // pointeur vers le Context actuel, pour faire les modifs de listen/server_name
		std::vector<std::pair<std::string, std::vector<std::string> > > _allowedInContext;

		/* utils global methods */
		void	keyNameCheck(const std::vector<std::pair<std::string, std::vector<std::string> > >& directives, int contextType) const;
		void	semicolonCheck(const std::vector<std::string>& v, const std::string& directive) const;
		void	validateMinimumArgs(const std::vector<std::string>& group, size_t minArgs, const std::string& directive) const;
		void	validateStrictArgsNb(const std::vector<std::string>& group, size_t exactNb, const std::string& directive) const;

		std::vector<std::vector<std::string> >	splitDirectiveGroups(const std::vector<std::string>& values, const std::string& directive) const;


		/* initialisation methods*/
		void	initAllowedContext(void);
		void	initValidators(void);


		/* specific directives checks*/
		void	validateClientMaxBodySize(const std::vector<std::string>& values) const;
		void	validateErrorPage(const std::vector<std::string>& values) const;

		void	validateServer(const std::vector<std::string>& group, const Context& context) const;
		void	validateLocation(const std::vector<std::string>& group, const Context& context) const;


		/* cannot be const method as they are modifying _currentContext */
		void	validateListen(const std::vector<std::string>& values);
		void	validateServerName(const std::vector<std::string>& values);

		void	validateRoot(const std::vector<std::string>& values) const;
		void	validateIndex(const std::vector<std::string>& values) const;
		void	validateAutoIndex(const std::vector<std::string>& values) const;
		void	validateAllowedMethods(const std::vector<std::string>& values) const;


		void	validateReturn(const std::vector<std::string>& values) const;

		/* CGI validation */
		void	validateCGIPath(const std::vector<std::string>& values) const;
		void	validateCGIExt(const std::vector<std::string>& values) const;

		void	validateCGIPairing(const Context& context) const;

		/* subdivision of directives checks */
		void	validateGlobalDirective(void) const;
		void	validateServerContexts(void);
		void	validateLocationContexts(Context& serverContext);

		void	validateContextDirectives(Context& context, int contextType);

		void	validateUploadTo(const std::vector<std::string>& values) const;
		void	validateAlias(const std::vector<std::string>& values) const;

		void	validatePostUploadToPairing(const Context& context) const;
		void	rejectAliasRootInSameLocation(const Context& context) const;

		// validateLocationContexts()
		void	validateContextDirectives(const Context& context, int contextType) const;
		/* member functions table pointer */
		typedef void (Validator::*DirectiveValidator)(const std::vector<std::string>&) const;
		std::map<std::string, DirectiveValidator> _directiveValidators;


		/* context parsing start */
		void	contextNameCheck(const Context& context, int expectedType) const;
		void	checkContextClosedProperly(const Context& context) const;


		/* utilitary functions to move out*/
		std::string	extractContextType(const std::string& contextName) const;
		bool		isOnlySemicolons(const std::string& str) const;

		/* listen utils */
		bool	isValidPort(std::string& portStr, int& outPort) const;
		bool	isValidAddress(std::string& address) const;

		void	subdivideListen(const std::string& listenValue) const;

		void	validateVirtualHostConflicts(void) const;


		void	rejectSoleBrackets(const std::vector<std::pair<std::string, std::vector<std::string> > >& directives) const;

	public:

		Validator(Config& config);
		~Validator();

		/* debug methods */
		void	printMap() const; /* name has to be changed for printPairs or something */
		void	printGroups(const std::vector<std::vector<std::string> >& groups) const;
		void	printVector(const std::vector<std::string>& v) const;

		/* maybe move this one to Utils namespace ? */
		std::vector<std::string>	createVectorFromString(const std::string& str) const;

		// main method, calls every subverification
		void	validate(void);
};

#endif
