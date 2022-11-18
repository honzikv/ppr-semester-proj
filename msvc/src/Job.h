#pragma once
#include "RunningStats.h"


/**
 * \brief Contains information about the processed job - i.e. which chunks to process, results of the processing, etc.
 */
struct Job {
	std::pair<size_t, size_t> ChunkIdxRange; // start index (inclusive) and end index (exclusive)
	RunningStats Result; // result of the processing - must be initialized by given DeviceCoordinator

	explicit Job(const std::pair<size_t, size_t> chunkIdxRange):
		ChunkIdxRange(chunkIdxRange) {
	}

};
