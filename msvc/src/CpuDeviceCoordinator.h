#pragma once
#include <filesystem>
#include <cmath>
#include "DeviceCoordinator.h"


namespace fs = std::filesystem;

class CpuDeviceCoordinator : public DeviceCoordinator {
public:
	CpuDeviceCoordinator(const CoordinatorType coordinatorType,
	                     const ProcessingMode processingMode,
	                     std::function<void(std::unique_ptr<Job>, size_t)> jobFinishedCallback,
	                     const size_t memoryLimit,
	                     const size_t chunkSizeBytes,
	                     fs::path& distFilePath,
	                     const size_t id,
	                     const size_t nCores = std::thread::hardware_concurrency()
	) : DeviceCoordinator(coordinatorType, processingMode, std::move(jobFinishedCallback), memoryLimit, chunkSizeBytes,
	                      distFilePath, id),
		nCores(nCores) {
		this->maxNumberOfChunks = static_cast<size_t>(floor(memoryLimit / chunkSizeBytes));

		startCoordinatorThread();
	}

protected:
	size_t nCores;
	void onProcessJob() override;
};
