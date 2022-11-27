#pragma once
#include <filesystem>
#include <cmath>

#include "ProcessingConfig.h"
#include "DeviceCoordinator.h"


namespace fs = std::filesystem;

class CpuDeviceCoordinator : public DeviceCoordinator {
public:
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
	size_t nCores{};
	void onProcessJob() override;
};
