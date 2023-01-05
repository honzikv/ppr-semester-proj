#define NOMINMAX
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
	maxNumberOfChunksPerJob = (cpuBufferSizeBytes / bytesPerAccumulator * bytesPerAccumulator) / chunkSizeBytes;
	if (processingMode == ProcessingMode::OPENCL_DEVICES) {
		return;
	}

	startCoordinatorThread();
}

void CpuDeviceCoordinator::onProcessJob() {
	log(INFO, "[SMP] Processing job with id " + std::to_string(currentJob->Id));
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);
	const auto doublesPerAccumulator = bytesPerAccumulator / sizeof(double);
	const auto nAccumulators = buffer.size() / doublesPerAccumulator;
	auto accumulators = std::vector<StatsAccumulator>(nAccumulators);
	
	if (nAccumulators <= 1) {
		// The job is too small to be processed in parallel
		// Therefore do it in a single thread
		log(DEBUG, "Job too small to be processed in parallel, processing in a single accumulator");
		auto accumulator = StatsAccumulator();
		for (auto i = 0ULL; i < buffer.size(); i += 1) {
			accumulator.push(buffer[i]);
		}
		accumulators.push_back(accumulator);
		notifyWatchdogCallback(buffer.size() * sizeof(double));
	}
	else {
		log(DEBUG, "[SMP] Job split into " + std::to_string(nAccumulators) + " accumulators");
		tbb::parallel_for(tbb::blocked_range<size_t>(0, nAccumulators),
		                  [&](const tbb::blocked_range<size_t> r) {
			                  for (auto accumulatorId = r.begin(); accumulatorId < r.end(); accumulatorId += 1) {
				                  const auto jobStart = accumulatorId * doublesPerAccumulator;
				                  const auto jobEnd = (accumulatorId + 1) * doublesPerAccumulator;
				                  for (auto i = jobStart; i < jobEnd; i += 1) {
					                  accumulators[accumulatorId].push(buffer[i]);
				                  }
								  notifyWatchdogCallback((jobEnd - jobStart) * sizeof(double));
			                  }
		                  });
	}
	
	currentJob->Items = accumulators;
	log(DEBUG,
	    "[SMP] Finished computing job with id " + std::to_string(currentJob->Id) + ". Computed " + std::to_string(
		    currentJob->getNChunks()) + " chunks. Chunk size is " + std::to_string(chunkSizeBytes) + " bytes");
}
