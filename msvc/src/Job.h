#pragma once
#include "RunningStats.h"


/**
 * \brief Contains information about the processed job - i.e. which chunks to process, results of the processing, etc.
 */
struct Job {
	const std::pair<size_t, size_t> ChunkIdxRange; // start index (inclusive) and end index (exclusive)
	std::vector<RunningStats> Result; // result of the processing
	const size_t Id;

	explicit Job(const std::pair<size_t, size_t> chunkIdxRange, const size_t id):
		ChunkIdxRange(chunkIdxRange),
		Id(id) {
	}

};
