#include <oneapi/tbb.h>
#include "Avx2StatsAccumulator.h"
#include "Avx2CpuDeviceCoordinator.h"
#include "StatUtils.h"


void Avx2CpuDeviceCoordinator::onProcessJob() {
	log(INFO, "Processing job with id " + std::to_string(currentJob->Id) + " on CPU / SMP ");
	// Load data into the vector
	auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	// Filter out invalid data
	// size_t currentIdx = 0;
	// for (auto i = 0ULL; i < buffer.size(); i += 1) {
	// 	if (StatUtils::valueNormalOrZero(buffer[i])) {
	// 		buffer[currentIdx] = buffer[i];
	// 		currentIdx += 1;
	// 	}
	// }
	// buffer.resize(currentIdx + 1);

	// Create vector for results
	const auto nAccumulators = buffer.size() / bytesPerAccumulator;
	auto accumulators = std::vector<Avx2StatsAccumulator>(nAccumulators);

	if (nAccumulators <= 1) {
		// The job is too small to be processed in parallel
		// Therefore do it in a single thread
		auto accumulator = Avx2StatsAccumulator();
		for (auto i = 0ULL; i < buffer.size() / 4; i += 4) {
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
		oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, nAccumulators),
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

	currentJob->Result = result;
}
