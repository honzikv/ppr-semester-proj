#include <tbb/tbb.h>

#include "DistributionClassification.h"
#include "JobScheduler.h"
#include "Logging.h"
#include "SingleThreadStatsComputation.h"
#include "StatUtils.h"
#include "Timer.h"
#include "ArgumentParser.h"

int main(int argc, char** argv) {
	// Parse arguments
	auto argParser = ArgumentParser();
	
	ProcessingConfig processingConfig;
	try {
		processingConfig = argParser.processArgs(argc, argv);
	}
	catch (const std::runtime_error err) {
		std::cout << err.what() << std::endl;
		exit(1);
	}
	// Create timer to measure benchmark
	auto timer = Timer();
	//
	// Configure TBB if needed
	auto tbbThreadControl = tbb::global_control(tbb::global_control::max_allowed_parallelism,
	                                            processingConfig.ProcessingMode == SINGLE_THREAD
		                                            ? 1
		                                            : tbb::this_task_arena::max_concurrency()
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
	const auto singleThreadComputation = SingleThreadStatsComputation(4096, processingConfig, 8, 256 * 1024 * 1024);
	auto results = singleThreadComputation.run(256ULL * 256 * sizeof(double));

	auto results2 = std::vector<StatsAccumulator>(results.size() / 2);
	for (auto i = 0; i < results.size() / 2; i += 1) {
		results2[i] = results[i*2] + results[i*2 + 1];
	}

	classifyDistribution(StatUtils::mergeLeftToRight(results));
	classifyDistribution(StatUtils::mergeLeftToRight(results2));
	timer.stop();
	timer.logResults();

	// openClTest();

	return 0;

}
