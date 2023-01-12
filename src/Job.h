#pragma once
#include "StatsAccumulator.h"


/**
 * \brief Contains information about the processed job - i.e. which chunks to process, results of the processing, etc.
 */
struct Job {
	const std::pair<size_t, size_t> ChunkIdxRange; // start index (inclusive) and end index (exclusive)
	std::vector<StatsAccumulator> Items; // result of the processing
	const size_t Id; // id of the job

	explicit Job(const std::pair<size_t, size_t> chunkIdxRange, const size_t id):
		ChunkIdxRange(chunkIdxRange),
		Id(id) {
	}

	/**
	 * \brief Returns size of the job in bytes
	 * \param chunkSizeBytes size of the chunk
	 * \return job size in bytes
	 */
	[[nodiscard]] auto getSizeBytes(const size_t chunkSizeBytes) const {
		return (ChunkIdxRange.second - ChunkIdxRange.first) * chunkSizeBytes;
	}

	/**
	 * \brief Returns number of chunks in this job
	 * \return number of chunks in this job
	 */
	[[nodiscard]] auto getNChunks() const {
		return ChunkIdxRange.second - ChunkIdxRange.first;
	}
};
