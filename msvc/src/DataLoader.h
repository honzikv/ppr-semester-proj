#pragma once
#include <fstream>
#include <filesystem>
#include <CL/cl.hpp>

#include "Job.h"

namespace fs = std::filesystem;

/**
 * \brief Simple class that wraps file reading
 */
class DataLoader {
private:
	std::ifstream file;

public:
	const size_t ChunkSizeBytes;

	/**
	 * \brief Creates new data loader
	 * \param filePath path to the file
	 * \param chunkSizeBytes size of one chunk, must be a multiple of sizeof(double)
	 */
	explicit DataLoader(const fs::path& filePath, const size_t chunkSizeBytes) : ChunkSizeBytes(chunkSizeBytes) {
		file.open(filePath, std::ios::in | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("Unable to open file: " + filePath.string());
		}
	}

private:
	/**
	 * \brief Computes metadata for given job
	 * \param job job to compute metadata for
	 * \return triplet of number of chunks, number of bytes to read, and starting address
	 */
	[[nodiscard]] auto computeJobMetadata(const Job& job) const {
		const auto [startIdx, endIdx] = job.ChunkIdxRange;
		const auto nChunks = endIdx - startIdx;
		const auto bytesToRead = nChunks * ChunkSizeBytes;
		const auto address = startIdx * ChunkSizeBytes;

		return std::make_tuple(nChunks, bytesToRead, address);
	}

public:
	/**
	 * \brief Loads all job data into buffer and returns it
	 * \param job job
	 * \return vector of doubles containing all loaded job data
	 */
	auto loadJobDataIntoVector(const Job& job) {
		const auto [nChunks, bytesToRead, address] = computeJobMetadata(job);

		// Create memory buffer, note that we assume that chunkSizeBytes is a multiple of sizeof(double)
		auto buffer = std::vector<double>(bytesToRead / sizeof(double));

		// Move to correct address in the file
		file.seekg(static_cast<int64_t>(address), std::ios::beg);

		// Read data into the buffer
		file.read(reinterpret_cast<char*>(buffer.data()), static_cast<int64_t>(bytesToRead));

		// Return the buffer
		return buffer;
	}

	/**
	 * \brief Loads job data into device buffer and returns the number of chunks in the device buffer
	 * \param job job to load
	 * \param maxHostChunks max number of chunks that can be loaded into host memory at a time
	 * \param commandQueue command queue for the given device - to write to the device buffer
	 * \param deviceBuffer reference to the device buffer
	 * \return number of chunks in the device buffer
	 */
	auto loadJobDataIntoDeviceBuffer(const Job& job,
	                                 const size_t maxHostChunks,
	                                 const cl::CommandQueue& commandQueue,
	                                 const cl::Buffer& deviceBuffer) {
		const auto [nChunks, _, address] = computeJobMetadata(job);
		auto hostBuffer = std::vector<double>(maxHostChunks * ChunkSizeBytes / sizeof(double));

		// Move to correct address in the file
		file.seekg(static_cast<int64_t>(address), std::ios::beg);

		auto chunksRemaining = nChunks;
		auto currentChunk = 0ULL;
		while (chunksRemaining > 0) {
			const auto chunksToRead = chunksRemaining < maxHostChunks ? chunksRemaining : maxHostChunks;
			const auto bytesToRead = chunksToRead * ChunkSizeBytes;

			// Read data into the buffer
			file.read(reinterpret_cast<char*>(hostBuffer.data()), static_cast<int64_t>(bytesToRead));

			// Copy host buffer to the device buffer
			commandQueue.enqueueWriteBuffer(deviceBuffer, CL_TRUE, currentChunk * ChunkSizeBytes,
			                                chunksToRead * ChunkSizeBytes * sizeof(double),
			                                hostBuffer.data());

			// Update
			currentChunk += chunksToRead;
			chunksRemaining -= chunksToRead;
		}

		// Return actual number of chunks
		return nChunks;
	}
};
