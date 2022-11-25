#pragma once
#include <filesystem>
#include <cmath>
#include "DeviceCoordinator.h"


namespace fs = std::filesystem;

class CpuDeviceCoordinator : public DeviceCoordinator {
public:
	CpuDeviceCoordinator(const CoordinatorType coordinatorType,
	                     const ProcessingMode processingMode,
	                     const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
	                     const size_t chunkSizeBytes,
	                     const size_t bytesPerAccumulator,
	                     const size_t cpuBufferSizeBytes,
	                     fs::path& distFilePath,
	                     const size_t id,
	                     const size_t nCores = std::thread::hardware_concurrency()
	) : DeviceCoordinator(
		coordinatorType, processingMode, jobFinishedCallback, 
		chunkSizeBytes, bytesPerAccumulator, distFilePath, id),
	    nCores(nCores) {
		maxNumberOfChunksPerJob = cpuBufferSizeBytes / chunkSizeBytes;
		startCoordinatorThread();
	}

protected:
	size_t nCores;
	void onProcessJob() override;
};
