#include <iostream>

#include "Arghandling.h"


int main(int argc, char** argv) {
	auto args = parseArguments(argc, argv);
	auto processingInfo = validateArguments(args);
	std::cout << std::endl;
}
