#include "Arghandling.h"
#include "DistributionClassification.h"
#include "JobScheduler.h"


int main(int argc, char** argv) {
	auto args = parseArguments(argc, argv);
	auto processingInfo = validateArguments(args);
	auto jobScheduler = JobScheduler(processingInfo);
	auto result = jobScheduler.run();
	
	// Classify the result
	classifyDistribution(result);

	// VectorizationUtils::testValuesInvalidFn();
	// VectorizationUtils::TestValuesIntegerFn();
	

	return 0;

}
