#include <oneapi/tbb.h>

#include "DistributionClassification.h"
#include "JobScheduler.h"
#include "Logging.h"
#include "SingleThreadStatsComputation.h"

// #include "OpenClTest.h"
#include "StatUtils.h"
#include "Timer.h"
#include "ArgParser.h"

int main(int argc, char** argv) {
	auto argParser = ArgumentParser();
	auto processingConfig = argParser.processArgs(argc, argv);

	auto timer = Timer();

	auto tbbThreadControl = oneapi::tbb::global_control(oneapi::tbb::global_control::max_allowed_parallelism,
	                                                    processingConfig.ProcessingMode == SINGLE_THREAD
		                                                    ? 1
		                                                    : oneapi::tbb::this_task_arena::max_concurrency()
	);

	timer.start();
	auto jobScheduler = JobScheduler(processingConfig);
	auto res = jobScheduler.run();

	// Classify the result
	classifyDistribution(StatUtils::mergeLeftToRight(res));

	timer.stop();
	timer.logResults();

	std::cout << "\nSingle Thread Test\n" << std::endl;
	timer.start();
	const auto singleThreadComputation = SingleThreadStatsComputation(4096, processingConfig, 8, 256 * 1024 * 1024 / 6);
	auto results = singleThreadComputation.run(256ULL * 256 * sizeof(double));
	classifyDistribution(StatUtils::mergeLeftToRight(results));
	timer.stop();
	timer.logResults();

	// openClTest();

	return 0;

}
