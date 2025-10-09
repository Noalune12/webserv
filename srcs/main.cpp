#include "main.hpp"

#include <iostream>
#include <cstdlib>

int	main(void) {

	Test test;

	std::cout << (test._value ? test._value : 100) << std::endl;

	/* main behavior:

	"Your program must use a configuration file, provided as an argument on the command line, or available in a default path."

	check if one arg	-> must be configuration file
	if no arg			-> launch server with own config file.

	*/

	return (EXIT_SUCCESS);
}
