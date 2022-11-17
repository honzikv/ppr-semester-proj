#pragma once
#include <vector>

#include "Arghandling.h"
#include "ClDeviceCoordinator.h"
#include "DeviceCoordinator.h"
#include "FileChunkHandler.h"
#include "SingleThreadDeviceCoordinator.h"
#include "SmpDeviceCoordinator.h"

constexpr auto MEMORY_LIMIT = 500 * 1024 * 1024;

class JobScheduler {

	/**
	 * \brief List of all device coordinators
	 */
	std::vector<std::shared_ptr<ClDeviceCoordinator>> clDeviceCoordinators;
	std::unique_ptr<SingleThreadDeviceCoordinator> singleThreadDeviceCoordinator = nullptr;
	std::unique_ptr<SmpDeviceCoordinator> smpDeviceCoordinator = nullptr;

	FileChunkHandler fileChunkHandler;

public:
	JobScheduler(ProcessingInfo processingInfo, size_t chunkSize = DEFAULT_CHUNK_SIZE, double percentRequired = 1.0):
		fileChunkHandler(processingInfo.distFilePath, percentRequired, chunkSize) {
		// Create coordinator for each device

		if (processingInfo.processingMode == ProcessingMode::SINGLE_THREAD) {
			singleThreadDeviceCoordinator = std::make_unique<SingleThreadDeviceCoordinator>(
				CoordinatorType::SINGLE_CORE,
				[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
				MEMORY_LIMIT,
				DEFAULT_CHUNK_SIZE,
				processingInfo.distFilePath
			);
			return;
		}

		// Add CL device if OpenCL mode
		if (processingInfo.processingMode == ProcessingMode::OPEN_CL_DEVICES) {
			for (const auto& [platform, devices] : processingInfo.devices) {
				for (const auto& device : devices) {
					clDeviceCoordinators.push_back(std::make_shared<ClDeviceCoordinator>(CoordinatorType::OPEN_CL,
							[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
							0, // TODO calculate memory limit
							0, // TODO calculate optimal chunk size
							processingInfo.distFilePath,
							platform,
							device)
					);
				}
			}
		}

		// Add SMP if SMP mode or use all devices
		if (processingInfo.processingMode == ProcessingMode::SMP
			|| processingInfo.processingMode == ProcessingMode::ALL) {
			smpDeviceCoordinator = std::make_unique<SmpDeviceCoordinator>(
				CoordinatorType::TBB,
				[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
				MEMORY_LIMIT,
				DEFAULT_CHUNK_SIZE,
				processingInfo.distFilePath
			);
		}

	}

	void jobFinishedCallback(Job job) {

	}
};
