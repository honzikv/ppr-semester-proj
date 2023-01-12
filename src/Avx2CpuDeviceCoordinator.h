#pragma once
#include "ProcessingConfig.h"
#include "CpuDeviceCoordinator.h"

/**
 * \brief Override for AVX2 capable CPU
 */
class Avx2CpuDeviceCoordinator final : public CpuDeviceCoordinator {

public:
	/**
	 * \brief Default constructor for the object
	 * \param coordinatorType type of the coordinator
	 * \param processingMode processing mode
	 * \param jobFinishedCallback callback when job is finished
	 * \param notifyWatchdogCallback callback to notify the watchdog
	 * \param errCallback error callback
	 * \param chunkSizeBytes chunk size in bytes
	 * \param bytesPerAccumulator number of bytes per single accumulator
	 * \param cpuBufferSizeBytes buffer size for a single job
	 * \param distFilePath path to the file being processed
	 * \param id id of this Device Coordinator
	 */
	Avx2CpuDeviceCoordinator(const CoordinatorType coordinatorType,
	                         const ProcessingMode processingMode,
	                         const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
	                         const std::function<void(size_t)>& notifyWatchdogCallback,
	                         const std::function<void(CoordinatorErr)>& errCallback,
	                         const size_t chunkSizeBytes,
	                         const size_t bytesPerAccumulator,
	                         const size_t cpuBufferSizeBytes,
	                         fs::path& distFilePath,
	                         const size_t id);

protected:
	void onProcessJob() override;
};
