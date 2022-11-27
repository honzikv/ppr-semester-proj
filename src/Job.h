#pragma once
#include "StatsAccumulator.h"


/**
 * \brief Contains information about the processed job - i.e. which chunks to process, results of the processing, etc.
 */
struct Job {
	const std::pair<size_t, size_t> ChunkIdxRange; // start index (inclusive) and end index (exclusive)
	std::vector<StatsAccumulator> Items; // result of the processing
	const size_t Id;
	bool DiscardRemainder = true;

	explicit Job(const std::pair<size_t, size_t> chunkIdxRange, const size_t id):
		ChunkIdxRange(chunkIdxRange),
		Id(id) {
	}

	[[nodiscard]] auto getSize(const size_t chunkSizeBytes) const {
		return (ChunkIdxRange.second - ChunkIdxRange.first) * chunkSizeBytes;
	}

	[[nodiscard]] auto getNChunks() {
		return ChunkIdxRange.second - ChunkIdxRange.first;
	}
};
