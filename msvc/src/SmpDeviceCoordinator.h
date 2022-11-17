#pragma once
#include "DeviceCoordinator.h"
#include <filesystem>

namespace fs = std::filesystem;

class SmpDeviceCoordinator : DeviceCoordinator {
public:
	SmpDeviceCoordinator(CoordinatorType coordinatorType, const std::function<void(Job)>& jobFinishedCallback,
	                     size_t memoryLimit, size_t chunkSize, fs::path& distFilePath)
		: DeviceCoordinator(coordinatorType, jobFinishedCallback, memoryLimit, chunkSize, distFilePath) {
	}

	~SmpDeviceCoordinator() override;
protected:
	void onProcessJob() override {

	}
};
