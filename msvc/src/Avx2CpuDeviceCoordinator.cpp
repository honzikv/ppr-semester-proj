#include <oneapi/tbb.h>
#include "Avx2StatsAccumulator.h"
#include "Avx2CpuDeviceCoordinator.h"


void Avx2CpuDeviceCoordinator::onProcessJob() {
	auto dataLoader = DataLoader(distFilePath, chunkSizeBytes);
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	// Create vector for results
	auto nJobs = nCores * 16;
	auto jobSize = buffer.size() / nJobs;
	auto statsAccumulators = std::vector<Avx2StatsAccumulator>(nJobs);

	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, nJobs),
	                          [&](const tbb::blocked_range<size_t> r) {
		                          for (auto i = r.begin(); i < r.end(); i += 1) {
			                          auto jobStart = i * jobSize / 4;
			                          auto jobEnd = (i + 1) * jobSize / 4;
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
