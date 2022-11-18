#include <iostream>
#include <oneapi/tbb.h>
#include "Arghandling.h"
#include "JobScheduler.h"
#include <oneapi/tbb.h>

int main(int argc, char** argv) {
	auto args = parseArguments(argc, argv);
	auto processingInfo = validateArguments(args);
	auto jobScheduler = JobScheduler(processingInfo);

	auto result = jobScheduler.run();

	return 0;
	
}
