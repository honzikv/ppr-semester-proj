#include <tbb/tbb.h>

#include "CpuDeviceCoordinator.h"
#include "Logging.h"


CpuDeviceCoordinator::CpuDeviceCoordinator(const CoordinatorType coordinatorType,
                                           const ProcessingMode processingMode,
                                           const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
                                           const std::function<void(size_t)>& notifyWatchdogCallback,
                                           const std::function<void(CoordinatorErr)>& errCallback,
                                           const size_t chunkSizeBytes,
                                           const size_t bytesPerAccumulator,
                                           const size_t cpuBufferSizeBytes,
                                           fs::path& distFilePath,
                                           const size_t id
) :
	DeviceCoordinator(
		coordinatorType,
		processingMode,
		jobFinishedCallback,
		notifyWatchdogCallback,
		errCallback,
		chunkSizeBytes,
		bytesPerAccumulator,
		distFilePath,
		id) {
	maxNumberOfChunksPerJob = cpuBufferSizeBytes / chunkSizeBytes;
	startCoordinatorThread();
}

void CpuDeviceCoordinator::onProcessJob() {
	log(INFO, "[SMP] Processing job with id " + std::to_string(currentJob->Id));
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	const auto nAccumulators = buffer.size() * sizeof(double) / bytesPerAccumulator;
	auto accumulators = std::vector<StatsAccumulator>(nAccumulators);
	if (nAccumulators <= 1) {
		// The job is too small to be processed in parallel
		// Therefore do it in a single thread
		auto accumulator = StatsAccumulator();
		for (auto i = 0ULL; i < buffer.size(); i += 1) {
			accumulator.push(buffer[i]);
		}
		accumulators.push_back(accumulator);
	}
	else {
		tbb::parallel_for(tbb::blocked_range<size_t>(0, nAccumulators),
		                  [&](const tbb::blocked_range<size_t> r) {
			                  for (auto accumulatorId = r.begin(); accumulatorId < r.end(); accumulatorId += 1) {
				                  const auto jobStart = accumulatorId * (bytesPerAccumulator / sizeof(double));
				                  const auto jobEnd = (accumulatorId + 1) * (bytesPerAccumulator / sizeof(double));
				                  for (auto i = jobStart; i < jobEnd; i += 1) {
					                  accumulators[accumulatorId].push(buffer[i]);
				                  }
			                  }
		                  });
	}
	
	notifyWatchdogCallback(currentJob->getSize(chunkSizeBytes));
	currentJob->Items = accumulators;

}
