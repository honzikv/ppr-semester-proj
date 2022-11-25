#include <oneapi/tbb.h>
#include "Avx2StatsAccumulator.h"
#include "Avx2CpuDeviceCoordinator.h"


void Avx2CpuDeviceCoordinator::onProcessJob() {
	auto dataLoader = DataLoader(filePath, chunkSizeBytes);

	// Load data into the vector
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	// Compute number of jobs per core / worker
	// For algorithm to work we need to have similar amount of elements in each stats accumulator


	// Create vector for results
	auto nAccumulators = buffer.size() / bytesPerAccumulator;
	auto statsAccumulators = std::vector<Avx2StatsAccumulator>(nAccumulators);

	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, nAccumulators),
	                          [&](const tbb::blocked_range<size_t> r) {
		                          for (auto i = r.begin(); i < r.end(); i += 1) {
			                          const auto jobStart = i * bytesPerAccumulator / 4;
			                          const auto jobEnd = (i + 1) * bytesPerAccumulator / 4;
			                          for (auto j = jobStart; j < jobEnd; j += 1) {
				                          statsAccumulators[i].push({
					                          buffer[j * 4],
					                          buffer[j * 4 + 1],
					                          buffer[j * 4 + 2],
					                          buffer[j * 4 + 3],
				                          });
			                          }
		                          }
	                          });

	auto result = std::vector<StatsAccumulator>();
	for (const auto& avx2StatsAccumulator : statsAccumulators) {
		result.push_back(avx2StatsAccumulator.asScalar());
	}

	currentJob->Result = result;

	std::cout << "Job Finished" << std::endl;
}
