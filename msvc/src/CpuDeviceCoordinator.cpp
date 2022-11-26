#include <oneapi/tbb.h>

#include "CpuDeviceCoordinator.h"
#include "Logging.h"


CpuDeviceCoordinator::CpuDeviceCoordinator(const CoordinatorType coordinatorType, const ProcessingMode processingMode,
                                           const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback, const size_t chunkSizeBytes,
                                           const size_t bytesPerAccumulator, const size_t cpuBufferSizeBytes, fs::path& distFilePath, const size_t id): DeviceCoordinator(
		                      coordinatorType, processingMode, jobFinishedCallback, 
		                      chunkSizeBytes, bytesPerAccumulator, distFilePath, id) {
	maxNumberOfChunksPerJob = cpuBufferSizeBytes / chunkSizeBytes;
	startCoordinatorThread();
}

void CpuDeviceCoordinator::onProcessJob() {
	log(INFO, "Processing job with id " + std::to_string(currentJob->Id) + " on CPU / SMP ");
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	const auto nAccumulators = buffer.size() / bytesPerAccumulator;
	auto accumulators = std::vector<StatsAccumulator>(nAccumulators);

	if (nAccumulators <= 1) {
		// The job is too small to be processed in parallel
		// Therefore do it in a single thread
		auto accumulator = StatsAccumulator();
		for (auto i = 0ULL; i < buffer.size(); i += 1) {
			accumulator.push(buffer[i]);
		}
	}
	else {
		oneapi::tbb::parallel_for(tbb::blocked_range<size_t>(0, nAccumulators),
			[&](const tbb::blocked_range<size_t> r) {
				for (auto accumulatorId = r.begin(); accumulatorId < r.end(); accumulatorId += 1) {
					const auto jobStart = accumulatorId * bytesPerAccumulator;
					const auto jobEnd = (accumulatorId + 1) * bytesPerAccumulator;
					for (auto i = jobStart; i < jobEnd; i += 1) {
						accumulators[accumulatorId].push(buffer[i]);
					}
				}
			});
	}

	auto result = std::vector<StatsAccumulator>();
	for (const auto& accumulator : accumulators) {
		result.push_back(accumulator);
	}

	currentJob->Result = result;
	
}
