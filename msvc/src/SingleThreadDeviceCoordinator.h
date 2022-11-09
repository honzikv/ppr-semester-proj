#pragma once
#include "DeviceCoordinator.h"
#include <filesystem>
#include <fstream>

#include "RunningStats.h"

namespace fs = std::filesystem;

class SingleThreadDeviceCoordinator final : DeviceCoordinator {
public:
	SingleThreadDeviceCoordinator(CoordinatorType coordinatorType, const std::function<void(Job)>& jobFinishedCallback,
		size_t memoryLimit, size_t chunkSize, __resharper_unknown_type& distFilePath)
		: DeviceCoordinator(coordinatorType, jobFinishedCallback, memoryLimit, chunkSize, distFilePath) {
	}

protected:
	void onProcessJob() override {
		// Create memory buffer of size memoryLimit
		auto buffer = std::vector<double>(memoryLimit / sizeof(double));

		// Open the file
		auto file = std::ifstream(distFilePath, std::ios::binary);
		if (!file.is_open()) {
			// Set result as unsuccessful and return
			currentJob->result = {};
			return;
		}

		// TODO maybe sorting the indices might be faster than randomly accessing the file?
		for (const auto& chunkIdx : currentJob->chunksToProcess) {
			file.seekg(chunkIdx * chunkSize, std::ios::beg);
			file.read(reinterpret_cast<char*>(buffer.data()), chunkSize);
		}

		auto runningStats = RunningStats();
		for (const auto& value : buffer) {
			if (std::fpclassify(value) != FP_NORMAL) {
				continue;
			}
			runningStats.push(value);
		}

		currentJob->result = {
			true,
			runningStats.getMean(),
			runningStats.getVariance(),
			runningStats.getSkewness(),
			runningStats.getKurtosis()
		};
	}
};
