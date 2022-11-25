#pragma once
#include "StatsAccumulator.h"


/**
 * \brief Contains information about the processed job - i.e. which chunks to process, results of the processing, etc.
 */
struct Job {
	const std::pair<size_t, size_t> ChunkIdxRange; // start index (inclusive) and end index (exclusive)
	std::vector<StatsAccumulator> Result; // result of the processing
	const size_t Id;

	explicit Job(const std::pair<size_t, size_t> chunkIdxRange, const size_t id):
		ChunkIdxRange(chunkIdxRange),
		Id(id) {
	}

	/**
	 * \brief Splits job into multiple smaller evenly sized jobs with or without a remainder
	 * \param chunksPerJob number of chunks per job
	 * \param discardRemainder whether to discard a remainder
	 *        i.e. if we have 10 chunks and want 3 chunks per job we will discard the last chunk. Defaults to true
	 * \return vector of jobs
	 */
	[[nodiscard]] auto split(const size_t chunksPerJob, const bool discardRemainder = true) const {
		auto jobs = std::vector<Job>();

		const auto totalChunks = ChunkIdxRange.second - ChunkIdxRange.first;
		const auto totalJobsWoRemainder = totalChunks / chunksPerJob;
		const auto remainderChunks = totalChunks % chunksPerJob;

		for (auto i = 0ULL; i < totalJobsWoRemainder; i += 1) {
			const auto startIdx = ChunkIdxRange.first + i * chunksPerJob;
			const auto endIdx = startIdx + chunksPerJob;
			jobs.emplace_back(std::make_pair(startIdx, endIdx), Id);
		}

		if (!discardRemainder && remainderChunks > 0) {
			const auto startIdx = ChunkIdxRange.first + totalJobsWoRemainder * chunksPerJob;
			const auto endIdx = startIdx + remainderChunks;
			jobs.emplace_back(std::make_pair(startIdx, endIdx), Id);
		}

		return jobs;
	}
};
