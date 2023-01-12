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

	/**
	 * \brief Default constructor for the object
	 * \param distFilePath path to the file that is being processed
	 * \param chunkSizeBytes chunk size in bytes
	 */
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
		return nextChunkIdx == chunkCount;
	}

	/**
	 * \brief Return next N chunks to process - as a pair of start and end index
	 * \param n number of chunks to add
	 * \return pair of start and end (exclusive) chunk index
	 */
	auto getNextNChunks(const size_t n) {
		const auto actualChunksAdded = nextChunkIdx + n > chunkCount ? chunkCount - nextChunkIdx : n;
		auto result = std::pair<size_t, size_t>(nextChunkIdx, nextChunkIdx + actualChunksAdded);
		nextChunkIdx += actualChunksAdded;
		return result;
	}

	/**
	 * \brief Returns size of chunk in bytes
	 * \return size of chunk in bytes
	 */
	auto getChunkSizeBytes() const {
		return chunkSizeBytes;
	}

private:
	/**
	 * \brief Filesystem path to the file
	 */
	fs::path distFilePath;

	/**
	 * \brief Size of the file
	 */
	size_t fileSize;

	/**
	 * \brief Total number of chunks
	 */
	size_t chunkCount;

	/**
	 * \brief Chunk size in bytes
	 */
	size_t chunkSizeBytes;

	/**
	 * \brief Index of the next chunk
	 */
	size_t nextChunkIdx = 0;

};
