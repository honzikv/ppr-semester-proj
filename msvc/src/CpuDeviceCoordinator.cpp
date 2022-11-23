#include <oneapi/tbb.h>
#include <fstream>

#include "CpuDeviceCoordinator.h"


void CpuDeviceCoordinator::onProcessJob() {
	std::cout << "OnProcessJob CPU" << std::endl;

	// Load buffer and create structure for running stats
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);
	auto runningStats = std::vector<RunningStats>(nCores);

	// Run parallel loop
	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, buffer.size()),
	                          [&](const tbb::blocked_range<size_t> r) {
		                          for (auto i = r.begin(); i < r.end(); i += 1) {
			                          // Get current thread number (0 - N_CPU_CORES) and push
			                          // the value to the corresponding running stats object
			                          runningStats[
										  oneapi::tbb::this_task_arena::current_thread_index()
			                          ].push(buffer[i]);
		                          }
	                          });

	// Merge all running stats
	// auto& mergedRunningStats = runningStats[0];
	// for (auto i = 1; i < static_cast<int>(runningStats.size()); i += 1) {
	// 	mergedRunningStats += runningStats[i];
	// }
	//
	// // Set the result
	// currentJob->Result = mergedRunningStats;

	currentJob->Result = runningStats;
}
