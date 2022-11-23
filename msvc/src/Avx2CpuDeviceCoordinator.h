#pragma once
#include "CpuDeviceCoordinator.h"

/**
 * \brief Override for AVX2 capable CPU
 */
class Avx2CpuDeviceCoordinator final : public CpuDeviceCoordinator {
	
public:
	Avx2CpuDeviceCoordinator(const CoordinatorType coordinatorType,
	                         const ProcessingMode processingMode,
	                         std::function<void(std::unique_ptr<Job>, size_t)> jobFinishedCallback,
	                         const size_t memoryLimit,
	                         const size_t chunkSize,
	                         fs::path& distFilePath,
	                         const size_t id,
	                         const size_t nCores = std::thread::hardware_concurrency())
		: CpuDeviceCoordinator(coordinatorType, processingMode, std::move(jobFinishedCallback), memoryLimit, chunkSize,
		                       distFilePath, id, nCores) {
	}

protected:
	void onProcessJob() override;
};
