#include "SmpDeviceCoordinator.h"
#include <oneapi/tbb.h>

const auto CPU_CORES = std::thread::hardware_concurrency();

void SmpDeviceCoordinator::onProcessJob() {
	// Create memory buffer
	auto buffer = std::vector<double>(maxNumberOfChunks * chunkSize);

	// Open the file
	auto file = std::ifstream(distFilePath, std::ios::binary);

	// Read all bytes to the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), maxNumberOfChunks * chunkSize);
	// NOLINT(bugprone-narrowing-conversions)

	// OneTBB will use all available cores so create a running stats object for each CPU core
	auto runningStats = std::vector<RunningStats>(CPU_CORES);

	// Now simply run the parallel for loop
	oneapi::tbb::parallel_for(tbb::blocked_range<int>(0, buffer.size()),
	                          [&](const tbb::blocked_range<int> r) {
		                          for (auto i = r.begin(); i < r.end(); ++i) {
			                          if (std::fpclassify(buffer[i]) != FP_NORMAL) {
				                          continue;
			                          }

			                          runningStats[
				                          oneapi::tbb::this_task_arena::current_thread_index()
			                          ].push(buffer[i]);

		                          }
	                          });

	// Merge all running stats
	auto mergedRunningStats = runningStats[0];

	for (auto i = 1; i < runningStats.size(); ++i) {
		mergedRunningStats += runningStats[i];
	}
}
