#pragma once
#include <vector>

#include "Arghandling.h"
#include "DeviceCoordinator.h"
#include "FileChunkHandler.h"

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
			deviceCoordinators.push_back(DeviceCoordinator());
		}

	}
};
