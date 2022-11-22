#include <oneapi/tbb.h>
#include "Avx2RunningStats.h"
#include "Avx2CpuDeviceCoordinator.h"
#include <fstream>

const auto CPU_CORES = std::thread::hardware_concurrency();

void Avx2CpuDeviceCoordinator::onProcessJob() {
	std::cout << "OnProcessJob AVX2" << std::endl;

	// Open the file
	auto file = std::ifstream(distFilePath, std::ios::binary);

	// Get indices from the job
	auto [start, end] = currentJob->ChunkIdxRange;

	// Move to correct address in the file
	file.seekg(start * chunkSizeBytes * sizeof(double), std::ios::beg);

	// Create memory buffer
	auto buffer = std::vector<double>((end - start) * chunkSizeBytes / sizeof(double));

	// Read all bytes to the buffer
	// NOLINT(bugprone-narrowing-conversions)
	file.read(reinterpret_cast<char*>(buffer.data()), (end - start) * chunkSizeBytes);

	// OneTBB will use all available cores so create a running stats object for each CPU core
	auto runningStats = std::vector<Avx2RunningStats>(CPU_CORES);

	// Run parallel loop
	const auto alignedBufferSize = buffer.size() - (buffer.size() % sizeof(double));
	oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, alignedBufferSize, sizeof(double)),
		[&](const tbb::blocked_range<size_t> r) {
			runningStats[oneapi::tbb::this_task_arena::current_thread_index()].push(
				_mm256_loadu_pd(buffer.data() + r.begin())
			);
		});

	// Merge all running stats
	auto mergedRunningStats = runningStats[0].asScalar();
	for (auto i = 1ULL; i < runningStats.size(); i += 1) {
		mergedRunningStats += runningStats[i].asScalar();
	}

	// Map to RunningStats object
	currentJob->Result = mergedRunningStats;
}