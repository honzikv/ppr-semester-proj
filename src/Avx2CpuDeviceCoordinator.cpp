#include <tbb/tbb.h>

#include "Avx2StatsAccumulator.h"
#include "Avx2CpuDeviceCoordinator.h"
#include "Logging.h"
#include "StatUtils.h"


void Avx2CpuDeviceCoordinator::onProcessJob() {
	log(INFO, "[SMP (AVX2)] Processing job with id " + std::to_string(currentJob->Id));
	// Load data into the vector
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	// Create vector for results
	const auto nAccumulators = buffer.size() / bytesPerAccumulator;
	auto accumulators = std::vector<Avx2StatsAccumulator>(nAccumulators);

	if (nAccumulators <= 1) {
		// The job is too small to be processed in parallel
		// Therefore do it in a single thread
		auto accumulator = Avx2StatsAccumulator();
		for (auto i = 0ULL; i < buffer.size() / 4; i += 1) {
			accumulator.pushWithFiltering({
				buffer[i * 4],
				buffer[i * 4 + 1],
				buffer[i * 4 + 2],
				buffer[i * 4 + 3],
			});
		}
		accumulators.push_back(accumulator);
	}
	else {
		tbb::parallel_for(tbb::blocked_range<size_t>(0, nAccumulators),
		                          [&](const tbb::blocked_range<size_t> r) {
			                          for (auto accumulatorId = r.begin(); accumulatorId < r.end(); accumulatorId += 1) {
				                          const auto jobStart = accumulatorId * bytesPerAccumulator / 4;
				                          const auto jobEnd = (accumulatorId + 1) * bytesPerAccumulator / 4;
				                          for (auto i = jobStart; i < jobEnd; i += 1) {
					                          accumulators[accumulatorId].pushWithFiltering({
						                          buffer[i * 4],
						                          buffer[i * 4 + 1],
						                          buffer[i * 4 + 2],
						                          buffer[i * 4 + 3],
					                          });
				                          }
			                          }
		                          });
	}

	// Notify the watchdog

	auto result = std::vector<StatsAccumulator>();
	for (const auto& accumulator : accumulators) {
		auto items = accumulator.asVectorOfScalars();
		result.insert(result.end(), items.begin(), items.end());
	}

	notifyWatchdogCallback(currentJob->getSize(chunkSizeBytes));
	currentJob->Items = result;
}
