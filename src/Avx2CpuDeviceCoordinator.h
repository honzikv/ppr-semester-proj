#pragma once
#include "ProcessingConfig.h"
#include "CpuDeviceCoordinator.h"

/**
 * \brief Override for AVX2 capable CPU
 */
class Avx2CpuDeviceCoordinator final : public CpuDeviceCoordinator {

public:
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
