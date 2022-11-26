#include <oneapi/tbb.h>
#include "Arghandling.h"
#include "DistributionClassification.h"
#include "JobScheduler.h"
#include <fstream>

#include "Avx2StatsAccumulator.h"
#include "Logging.h"
#include "SingleThreadStatsComputation.h"

#include "OpenClTest.h"
#include "StatUtils.h"
#include "VectorizationUtils.h"
#include "Timer.h"

int main(int argc, char** argv) {
	accuTest();

	auto timer = Timer();
	auto args = parseArguments(argc, argv);
	auto processingInfo = validateArguments(args);

	auto tbbThreadControl = oneapi::tbb::global_control(oneapi::tbb::global_control::max_allowed_parallelism,
	                                                    processingInfo.ProcessingMode == SINGLE_THREAD
		                                                    ? 1
		                                                    : oneapi::tbb::this_task_arena::max_concurrency()
	);

	timer.start();
	auto jobScheduler = JobScheduler(processingInfo);
	auto res = jobScheduler.run();
	std::cout << "Result takes approx " << res.size() * sizeof(StatsAccumulator) << " bytes" << std::endl;
	// Classify the result
	classifyDistribution(StatUtils::mergeLeftToRight(res));
	timer.stop();
	timer.logResults();

	std::cout << "\nSingle Thread Test\n" << std::endl;
	timer.start();
	const auto singleThreadComputation = SingleThreadStatsComputation(4096, args, 8, 256 * 1024 * 1024 / 6);
	auto results = singleThreadComputation.run(256ULL * 256 * sizeof(double));
	classifyDistribution(StatUtils::mergeLeftToRight(results));
	timer.stop();
	timer.logResults();


	// openClTest();

	return 0;

}
