#include "main.hpp"

#include <iostream>
#include <cstdlib>

int	main(void) {

	Test test;

	std::cout << (test._value ? test._value : 100) << std::endl;

	return (EXIT_SUCCESS);
}
