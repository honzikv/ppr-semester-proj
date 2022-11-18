#pragma once
#include <filesystem>
#include <cmath>
#include "DeviceCoordinator.h"


namespace fs = std::filesystem;

class CpuDeviceCoordinator final : public DeviceCoordinator {
public:
	CpuDeviceCoordinator(const CoordinatorType coordinatorType,
	                     const ProcessingMode processingMode,
	                     const std::function<void(std::unique_ptr<Job>)>& jobFinishedCallback,
	                     const size_t memoryLimit,
	                     const size_t chunkSize,
	                     fs::path& distFilePath,
	                     size_t id
	) : DeviceCoordinator(coordinatorType, processingMode, jobFinishedCallback, memoryLimit, chunkSize,
	                     distFilePath, id) {
		this->maxNumberOfChunks = static_cast<size_t>(floor(memoryLimit / chunkSize));
	}

protected:
	void onProcessJob() override;
};
