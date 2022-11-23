#include <oneapi/tbb.h>
#include "Avx2RunningStats.h"
#include "Avx2CpuDeviceCoordinator.h"
#include <fstream>
#include <unordered_set>

const auto CPU_CORES = std::thread::hardware_concurrency();

std::atomic<size_t> nProcessed = 0;

void Avx2CpuDeviceCoordinator::onProcessJob() {
	std::cout << "OnProcessJob AVX2 CPU" << std::endl;

	// Open the file
	auto file = std::ifstream(distFilePath, std::ios::binary);

	// Get indices from the job
	auto [start, end] = currentJob->ChunkIdxRange;

	// Move to the correct address in the file
	file.seekg(start * chunkSizeBytes * sizeof(double), std::ios::beg);

	// Create memory buffer
	// We need to allocate (end - start) * chunkSize / sizeof(double) bytes
	auto buffer = std::vector<double>((end - start) * chunkSizeBytes / sizeof(double));

	// Read all bytes to the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), (end - start) * chunkSizeBytes);

	// Filter out invalid values
	size_t validIdx = 0;
	for (const auto& value : buffer) {
		if (const auto fpClass = std::fpclassify(value); fpClass == FP_NORMAL || fpClass == FP_ZERO) {
			buffer[validIdx] = value;
			validIdx += 1;
		}
	}

	// Resize the buffer to the number of valid values and align it for AVX2
	const auto nValidValues = validIdx + 1;
	buffer.resize(nValidValues - nValidValues % 4);

	// Create vector for results
	auto runningStats = std::vector<Avx2RunningStats>(nCores);
	auto mutex = std::mutex();

	auto indices = std::unordered_set<size_t>();

	// Run parallel_for
	// We divide the max range by four, because our vectors hold 4 doubles
	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, buffer.size() / 4),
	                          [&](const oneapi::tbb::blocked_range<size_t> range) {
		                          const auto threadIdx = oneapi::tbb::this_task_arena::current_thread_index();
		                          for (auto i = range.begin(); i < range.end(); i += 1) {
			                          runningStats[threadIdx].push({
				                          // Add items - indices are scaled back to the original index space
				                          buffer[i * 4],
				                          buffer[i * 4 + 1],
				                          buffer[i * 4 + 2],
				                          buffer[i * 4 + 3],
			                          });
		                          }
	                          });

	// Merge the results
	auto mergedRunningStats = runningStats[0].asScalar();
	for (auto i = 0; i < static_cast<int>(runningStats.size()); i += 1) {
		mergedRunningStats += runningStats[i].asScalar();
	}

	currentJob->Result = mergedRunningStats;
	nProcessed += currentJob->Result.getN();
	std::cout << "Processed total: " << nProcessed << " values" << std::endl;
}
