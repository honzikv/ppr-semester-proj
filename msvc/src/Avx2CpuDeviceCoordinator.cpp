#include <oneapi/tbb.h>
#include "Avx2RunningStats.h"
#include "Avx2CpuDeviceCoordinator.h"

void Avx2CpuDeviceCoordinator::onProcessJob() {
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	const auto jobSize = currentJob->size(dataLoader.ChunkSizeBytes);
	std::cout << "Job size: " << jobSize << std::endl;

	// Create vector for results
	auto runningStats = std::vector<Avx2RunningStats>(nCores);

	auto mutex = std::mutex{};
	// Run parallel_for
	// We divide the max range by four, because our vectors hold 4 doubles
	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, buffer.size() / 4),
	                          [&](const oneapi::tbb::blocked_range<size_t> range) {
		                          const auto threadIdx = oneapi::tbb::this_task_arena::current_thread_index();
		                          for (auto i = range.begin(); i < range.end(); i += 1) {
			                          auto idx = i * 4;
			                          runningStats[threadIdx].push({
				                          // Add items - indices are scaled back to the original index space
				                          buffer[idx],
				                          buffer[idx + 1],
				                          buffer[idx + 2],
				                          buffer[idx + 3]
			                          });
		                          }
	                          });

	auto result = std::vector<RunningStats>();
	for (const auto& avx2RunningStats : runningStats) {
		result.push_back(avx2RunningStats.asScalar());
	}

	currentJob->Result = result;

	std::cout << "Job Finished" << std::endl;
}
