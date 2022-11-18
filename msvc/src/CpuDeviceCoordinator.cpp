#include <oneapi/tbb.h>
#include <fstream>

#include "CpuDeviceCoordinator.h"


const auto CPU_CORES = std::thread::hardware_concurrency();

void CpuDeviceCoordinator::onProcessJob() {
	// Create memory buffer
	auto buffer = std::vector<double>(maxNumberOfChunks * chunkSize);

	// Open the file
	auto file = std::ifstream(distFilePath, std::ios::binary);

	// Get indices from the job
	auto [start, end] = currentJob->ChunkIdxRange;

	// Move to correct address in the file
	file.seekg(start * chunkSize * sizeof(double), std::ios::beg);

	// Read all bytes to the buffer
	// NOLINT(bugprone-narrowing-conversions)
	file.read(reinterpret_cast<char*>(buffer.data()), (end - start) * chunkSize);

	// OneTBB will use all available cores so create a running stats object for each CPU core
	auto runningStats = std::vector<RunningStats>(CPU_CORES);

	// Run parallel loop
	oneapi::tbb::parallel_for(tbb::blocked_range<int>(0, buffer.size()),
	                          [&](const tbb::blocked_range<int> r) {
		                          for (auto i = r.begin(); i < r.end(); i += 1) {
			                          // Classify the double if it is not "normal" continue
			                          if (std::fpclassify(buffer[i]) != FP_NORMAL) {
				                          continue;
			                          }

			                          // Get current thread number (0 - N_CPU_CORES) and push
			                          // the value to the corresponding running stats object
			                          runningStats[
				                          oneapi::tbb::this_task_arena::current_thread_index()
			                          ].push(buffer[i]);

		                          }
	                          });

	// Merge all running stats
	auto& mergedRunningStats = runningStats[0];

	for (auto i = 1; i < static_cast<int>(runningStats.size()); i += 1) {
		mergedRunningStats += runningStats[i];
	}

	// Set the result
	currentJob->Result = mergedRunningStats;
}
