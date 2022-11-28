#pragma once
#include <filesystem>

#include "ProcessingConfig.h"
#include "DeviceCoordinator.h"


namespace fs = std::filesystem;

/**
 * \brief This class is a base implementation for processing data on SMP - it does not support AVX2 and uses tbb threads
 *        to compute the data
 */
class CpuDeviceCoordinator : public DeviceCoordinator {
public:
	/**
	 * \brief Creates new CPU device coordinator instance
	 * \param coordinatorType  type of the coordinator
	 * \param processingMode mode of processing
	 * \param jobFinishedCallback callback after job is finished
	 * \param notifyWatchdogCallback callback for notifying watchdog
	 * \param errCallback callback for error handling
	 * \param chunkSizeBytes size of the chunk in bytes
	 * \param bytesPerAccumulator number of bytes processed by each StatsAccumulator
	 * \param cpuBufferSizeBytes buffer size in bytes
	 * \param distFilePath path to the file that is being processed
	 * \param id id of this coordinator
	 */
	CpuDeviceCoordinator(CoordinatorType coordinatorType,
	                     ProcessingMode processingMode,
	                     const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
	                     const std::function<void(size_t)>& notifyWatchdogCallback,
	                     const std::function<void(CoordinatorErr)>& errCallback,
	                     size_t chunkSizeBytes,
	                     size_t bytesPerAccumulator,
	                     size_t cpuBufferSizeBytes,
	                     fs::path& distFilePath,
	                     size_t id
	);

protected:
	/**
	 * \brief Override for CPU device without AVX2
	 */
	void onProcessJob() override;
};
