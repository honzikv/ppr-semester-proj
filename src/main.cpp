#include "DistributionClassification.h"
#include "JobScheduler.h"
#include "Logging.h"
#include "SingleThreadStatsComputation.h"
#include "StatUtils.h"
#include "Timer.h"
#include "ArgumentParser.h"
#include "Benchmark.h"

void run(ProcessingConfig& processingConfig) {
	log(INFO, "Processing file: \"" + processingConfig.DistFilePath.string() + "\"");
	// Configure TBB if needed
	auto tbbThreadControl = tbb::global_control(tbb::global_control::max_allowed_parallelism,
		processingConfig.ProcessingMode == SINGLE_THREAD
		? 1
		: tbb::this_task_arena::max_concurrency()
	);
	if (processingConfig.ProcessingMode == SINGLE_THREAD) {
		log(DEBUG, "Using only single thread on SMP");
	}

	try {
		auto jobScheduler = JobScheduler(processingConfig);
		setupOutputFileDirsIfNeeded(processingConfig);

		auto timer = Timer();
		timer.start();
		const auto result = jobScheduler.run();
		timer.stop();

		classifyDistribution(StatUtils::mergeLeftToRight(result));

		// If output file is not empty write the results to it as well
		if (!processingConfig.OutputPath.empty()) {
			auto file = std::fstream(processingConfig.OutputPath, std::ios::out);
			classifyDistribution(StatUtils::mergeLeftToRight(result), file);
		}

		timer.printResults();
	}
	catch (std::runtime_error& err) {
		log(CRITICAL, err.what());
		exit(1);  // NOLINT(concurrency-mt-unsafe)
	}
}

int main(int argc, char** argv) {
	// Parse arguments
	constexpr auto argParser = ArgumentParser();
	
	ProcessingConfig processingConfig;
	try {
		processingConfig = argParser.processArgs(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::cout << err.what() << std::endl;
		exit(1);  // NOLINT(concurrency-mt-unsafe)
	}

	if (processingConfig.IsBenchmark) {
		// Run benchmark
		runBenchmark(processingConfig);
		return 0;
	}

	// Otherwise run normally
	run(processingConfig);
	
	return 0;

}
