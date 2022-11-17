#pragma once
#include "SmpDeviceCoordinator.h"
#include "Arghandling.h"
#include "ClDeviceCoordinator.h"
#include "FileChunkHandler.h"
#include "SingleThreadDeviceCoordinator.h"

constexpr auto MEMORY_LIMIT = 500 * 1024 * 1024;
constexpr auto DEFAULT_CHUNK_SIZE = 4096;

class JobScheduler {

	/**
	 * \brief List of all device coordinators
	 */
	std::vector<std::shared_ptr<ClDeviceCoordinator>> clDeviceCoordinators;
	std::unique_ptr<SingleThreadDeviceCoordinator> singleThreadDeviceCoordinator = nullptr;
	std::unique_ptr<SmpDeviceCoordinator> smpDeviceCoordinator = nullptr;

	FileChunkHandler fileChunkHandler;

public:
	JobScheduler(ProcessingInfo processingInfo, size_t chunkSize = DEFAULT_CHUNK_SIZE):
		fileChunkHandler(processingInfo.DistFilePath, chunkSize) {
		// Depending on the mode create specific device coordinators

		if (processingInfo.ProcessingMode == ProcessingMode::SINGLE_THREAD) {
			singleThreadDeviceCoordinator = std::make_unique<SingleThreadDeviceCoordinator>(
				CoordinatorType::SINGLE_CORE,
				[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
				MEMORY_LIMIT,
				DEFAULT_CHUNK_SIZE,
				processingInfo.DistFilePath
			);
			return;
		}

		// Add CL device if OpenCL mode
		if (processingInfo.ProcessingMode == ProcessingMode::OPEN_CL_DEVICES) {
			for (const auto& [platform, devices] : processingInfo.Devices) {
				for (const auto& device : devices) {
					clDeviceCoordinators.push_back(std::make_shared<ClDeviceCoordinator>(CoordinatorType::OPEN_CL,
							[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
							0, // TODO calculate memory limit
							0, // TODO calculate optimal chunk size
							processingInfo.DistFilePath,
							platform,
							device)
					);
				}
			}
		}

		// Add SMP if SMP mode or use all devices
		if (processingInfo.ProcessingMode == ProcessingMode::SMP
			|| processingInfo.ProcessingMode == ProcessingMode::ALL) {
			smpDeviceCoordinator = std::make_unique<SmpDeviceCoordinator>(
				CoordinatorType::TBB,
				[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
				MEMORY_LIMIT,
				DEFAULT_CHUNK_SIZE,
				processingInfo.DistFilePath
			);
		}

	}

	void jobFinishedCallback(Job job) {

	}

	/**
	 * \brief Runs the job scheduler.
	 */
	inline auto run() {
		// while (true) {
		// 	// First check if there is a job available
		// 	if (!jobAvailable()) {
		// 		// If no job is available there might still
		// 		// be some devices computing
		// 		if (anyDeviceRunning()) {
		// 			waitForCoordinatorNotification();
		// 			continue;
		// 		}
		//
		// 		// If not terminate the loop
		// 		break;
		// 	}
		//
		// 	// Check if all devices are currently working,
		// 	// if so go to sleep and wait until some DeviceCoordinator notifies us
		// 	if (!anyDeviceAvailable()) {
		// 		waitForCoordinatorNotification();
		// 		continue;
		// 	}
		//
		// 	// Get the device coordinator
		// 	auto deviceCoordinator = getAvailableDeviceCoordinator();
		//
		// 	// Create new job
		// 	auto job = 
		// }
	}
};
