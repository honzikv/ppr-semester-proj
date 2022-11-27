#pragma once
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace fs = std::filesystem;

class FileChunkHandler {

public:
	// File is split into evenly sized chunks which are read by given device
	const size_t ChunkSizeBytes;

	FileChunkHandler(fs::path distFilePath, const size_t chunkSizeBytes) :
		ChunkSizeBytes(chunkSizeBytes),
		distFilePath(std::move(distFilePath)),
		fileSize(fs::file_size(this->distFilePath)),
		// We throw away the last chunk if it is smaller than chunkSizeBytes
		// The thrown away data are small enough so it won't affect the derived distribution
		chunkCount(static_cast<size_t>(floor(static_cast<double>(fileSize) / static_cast<double>(chunkSizeBytes)))),
		chunkSizeBytes(chunkSizeBytes) {
	}

	[[nodiscard]] bool allChunksProcessed() const {
		return currentChunkIdx == chunkCount;
	}

	/**
	 * \brief Return next N chunks to process - as a pair of start and end index
	 * \param n number of chunks to add
	 * \return pair of start and end (exclusive) chunk index
	 */
	auto getNextNChunks(const size_t n) {
		const auto actualChunksAdded = currentChunkIdx + n > chunkCount ? chunkCount - currentChunkIdx : n;
		auto result = std::pair<size_t, size_t>(currentChunkIdx, currentChunkIdx + actualChunksAdded);
		currentChunkIdx += actualChunksAdded;
		return result;
	}

	auto getChunkSizeBytes() const {
		return chunkSizeBytes;
	}

private:
	fs::path distFilePath;
	size_t fileSize;
	size_t chunkCount; // Total number of chunks
	size_t chunkSizeBytes; // Size of chunk in bytes
	size_t currentChunkIdx = 0; // Number of current chunks

};
