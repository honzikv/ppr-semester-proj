#pragma once
#include <filesystem>
#include <cmath>
#include <fstream>
#include "DeviceCoordinator.h"
#include "RunningStats.h"


namespace Fs = std::filesystem;

class SmpDeviceCoordinator : DeviceCoordinator {
public:
	SmpDeviceCoordinator(CoordinatorType coordinatorType, const std::function<void(Job)>& jobFinishedCallback,
	                     size_t memoryLimit, size_t chunkSize, Fs::path& distFilePath)
		: DeviceCoordinator(coordinatorType, jobFinishedCallback, memoryLimit, chunkSize, distFilePath) {
		this->maxNumberOfChunks = floor(memoryLimit / chunkSize);
	}
	
protected:
	void onProcessJob() override;
};
