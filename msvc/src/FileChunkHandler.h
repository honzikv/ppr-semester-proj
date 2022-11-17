#pragma once
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace Fs = std::filesystem;

class FileChunkHandler {

public:
	// File is split into evenly sized chunks which are read by given device
	const size_t ChunkSize;

	FileChunkHandler(Fs::path distFilePath, const size_t chunkSize) :
		ChunkSize(chunkSize),
		distFilePath(std::move(distFilePath)),
		fileSize(Fs::file_size(this->distFilePath)),
		// We throw away the last chunk if it is smaller than chunkSize
		// The thrown away data are small enough so it won't affect the derived distribution
		chunkCount(static_cast<uint32_t>(floor(static_cast<double>(fileSize) / static_cast<double>(chunkSize)))) {
	}

	inline [[nodiscard]] bool finished() {
		return currentChunkIdx == chunkCount - 1;
	}

	inline std::vector<uint32_t> getNextNChunks(const uint32_t n) {
		const auto actualChunksAdded = currentChunkIdx + n > chunkCount ? chunkCount - currentChunkIdx : n;
		auto result = std::vector<uint32_t>(actualChunksAdded);

		// Add the next n chunks to the result, update current chunk index and return
		std::iota(result.begin(), result.end(), currentChunkIdx);
		currentChunkIdx += actualChunksAdded;
		return result;
	}

private:
	Fs::path distFilePath;
	size_t fileSize;
	uint32_t chunkCount; // Total number of chunks
	uint32_t currentChunkIdx = 0; // Number of current chunks

};
