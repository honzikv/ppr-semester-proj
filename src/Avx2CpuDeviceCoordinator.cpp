#define NOMINMAX
#include <tbb/tbb.h>
#include "Avx2StatsAccumulator.h"
#include "Avx2CpuDeviceCoordinator.h"
#include "Logging.h"
#include "StatUtils.h"


Avx2CpuDeviceCoordinator::Avx2CpuDeviceCoordinator(const CoordinatorType coordinatorType,
                                                   const ProcessingMode processingMode,
                                                   const std::function<void(std::unique_ptr<Job>, size_t)>&
                                                   jobFinishedCallback,
                                                   const std::function<void(size_t)>& notifyWatchdogCallback,
                                                   const std::function<void(CoordinatorErr)>& errCallback,
                                                   const size_t chunkSizeBytes, const size_t bytesPerAccumulator,
                                                   const size_t cpuBufferSizeBytes,
                                                   fs::path& distFilePath, const size_t id): CpuDeviceCoordinator(
	coordinatorType,
	processingMode,
	jobFinishedCallback,
	notifyWatchdogCallback,
	errCallback,
	chunkSizeBytes,
	bytesPerAccumulator,
	cpuBufferSizeBytes,
	distFilePath,
	id) {
}

void Avx2CpuDeviceCoordinator::onProcessJob() {
	log(INFO, "[SMP (AVX2)] Processing job with id " + std::to_string(currentJob->Id));
	// Load data into the vector
	const auto buffer = dataLoader.loadJobDataIntoVector(*currentJob);

	// Create vector for results
	const auto doublesPerAccumulator = bytesPerAccumulator / sizeof(double);
	const auto nAccumulators = buffer.size() / doublesPerAccumulator;
	auto accumulators = std::vector<Avx2StatsAccumulator>(nAccumulators);

	if (nAccumulators <= 1) {
		// The job is too small to be processed in parallel
		// Therefore do it in a single thread
		log(DEBUG, "[SMP (AVX2)] Job too small to be processed in parallel, processing in a single accumulator");
		auto accumulator = Avx2StatsAccumulator();
		for (auto i = 0ULL; i < buffer.size() / 4; i += 1) {
			accumulator.pushWithFiltering({
				buffer[i * 4],
				buffer[i * 4 + 1],
				buffer[i * 4 + 2],
				buffer[i * 4 + 3],
			});
		}
		notifyWatchdogCallback(buffer.size() * sizeof(double));
		accumulators.push_back(accumulator);
	}
	else {
		log(DEBUG, "[SMP (AVX2)] Job split into " + std::to_string(nAccumulators) + " accumulators");
		tbb::parallel_for(tbb::blocked_range<size_t>(0, nAccumulators),
		                  [&](const tbb::blocked_range<size_t> r) {
			                  for (auto accumulatorId = r.begin(); accumulatorId < r.end(); accumulatorId += 1) {
				                  // Since we are using AVX2 in each step we process 4 doubles at once, therefore the indices must
				                  // be scaled by 1/4th
				                  const auto jobStart = (accumulatorId * doublesPerAccumulator) / 4;
				                  const auto jobEnd = jobStart + (doublesPerAccumulator) / 4;
				                  for (auto i = jobStart; i < jobEnd; i += 1) {
					                  accumulators[accumulatorId].pushWithFiltering({
						                  buffer[i * 4],
						                  buffer[i * 4 + 1],
						                  buffer[i * 4 + 2],
						                  buffer[i * 4 + 3],
					                  });
				                  }

				                  // Notify the watchdog
								  notifyWatchdogCallback((jobEnd - jobStart) * sizeof(double));
			                  }
		                  });
	}
	auto result = std::vector<StatsAccumulator>();
	for (const auto& accumulator : accumulators) {
		auto items = accumulator.asVectorOfScalars();
		result.insert(result.end(), items.begin(), items.end());
	}
	
	currentJob->Items = result;
	log(DEBUG,
	    "[SMP (AVX2)] Finished computing job with id " + std::to_string(currentJob->Id) + ". Computed " +
	    std::to_string(
		    currentJob->getNChunks()) + " chunks. Chunk size is " + std::to_string(chunkSizeBytes) + " bytes");
}
