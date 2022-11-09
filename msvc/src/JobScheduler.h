#pragma once
#include <vector>

#include "Arghandling.h"
#include "DeviceCoordinator.h"
#include "FileChunkHandler.h"
#include "SingleThreadDeviceCoordinator.h"

constexpr auto MEMORY_LIMIT = 500 * 1024 * 1024;

class JobScheduler {

	/**
	 * \brief List of all device coordinators
	 */
	std::vector<DeviceCoordinator> deviceCoordinators;

	FileChunkHandler fileChunkHandler;

public:
	JobScheduler(ProcessingInfo processingInfo, size_t chunkSize = DEFAULT_CHUNK_SIZE, double percentRequired = 1.0):
		fileChunkHandler(processingInfo.distFilePath, percentRequired, chunkSize) {
		// Create coordinator for each device

		if (processingInfo.processingMode == ProcessingMode::SINGLE_THREAD) {
			auto singleThreadCoordinator = SingleThreadDeviceCoordinator(
				CoordinatorType::SINGLE_CORE,
				jobFinishedCallback,
				MEMORY_LIMIT,
				DEFAULT_CHUNK_SIZE,
				processingInfo.distFilePath
				);
		}

	}

	void jobFinishedCallback(Job job) {
		
	}
};
