#ifndef CONFIGINHERITOR_HPP
# define CONFIGINHERITOR_HPP

/*
 * Gestion de l'héritage des directives de configuration
 *
 * Responsabilités:
 * - Applique l'héritage des directives: global -> server -> location
 * - Les directives définies au niveau global sont héritées par tous les blocs server
 * - Les directives définies dans server sont héritées par tous les blocs location
 * - Les directives locales surchargent les directives héritées (override)
 */
class ConfigInheritor {

	private:

	public:
		ConfigInheritor();
		~ConfigInheritor();

};

#endif
