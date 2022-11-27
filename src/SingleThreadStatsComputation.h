#pragma once
#include "FileChunkHandler.h"
#include <filesystem>
#include "ProcessingConfig.h"
#include "DataLoader.h"


namespace fs = std::filesystem;

/**
 * \brief Debug class to measure performance and test several merging approaches to StatsAccumulator class
 */
class SingleThreadStatsComputation {

	size_t chunkSizeBytes;
	size_t maxBufferSize;
	size_t nWorkers;
	size_t defaultMaxSizePerAccumulator;
	ProcessingConfig processingConfig;

public:
	SingleThreadStatsComputation(const size_t chunkSize,
	                             ProcessingConfig processingArgs,
	                             const size_t nWorkers,
	                             const size_t maxBufferSize):
		chunkSizeBytes(chunkSize),
		maxBufferSize(maxBufferSize),
		nWorkers(nWorkers),
		defaultMaxSizePerAccumulator(maxBufferSize / nWorkers),
		processingConfig(std::move(processingArgs)) {
	}

	[[nodiscard]] auto run(int64_t maxBytesPerAccumulator = -1) const {
		if (maxBytesPerAccumulator == -1 || static_cast<size_t>(maxBytesPerAccumulator) >
			defaultMaxSizePerAccumulator) {
			maxBytesPerAccumulator = static_cast<int64_t>(defaultMaxSizePerAccumulator);
		}

		// Create jobs
		auto fileChunkHandler = FileChunkHandler(processingConfig.DistFilePath, chunkSizeBytes);
		auto dataLoader = DataLoader(processingConfig.DistFilePath, chunkSizeBytes);
		auto results = std::vector<StatsAccumulator>();
		const auto chunksPerJob = maxBytesPerAccumulator / chunkSizeBytes;
		auto id = 0;

		// Until file isn't processed run one job at a time
		while (!fileChunkHandler.allChunksProcessed()) {
			const auto [start, end] = fileChunkHandler.getNextNChunks(chunksPerJob);
			if (start - end < chunksPerJob) {
				break;
			}

			const auto job = Job({start, end}, id++);
			const auto buffer = dataLoader.loadJobDataIntoVector(job);

			auto result = StatsAccumulator();
			for (const auto& x : buffer) {
				result.push(x);
			}

			results.push_back(result);
		}

		return results;
	}

	
};
