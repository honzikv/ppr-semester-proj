#pragma once

#include <fstream>
#include <filesystem>

#define NOMINMAX
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>

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
	explicit DataLoader(const fs::path& filePath, const size_t chunkSizeBytes);

	/**
	 * \brief Loads all job data into buffer and returns it
	 * \param job job
	 * \return vector of doubles containing all loaded job data
	 */
	std::vector<double> loadJobDataIntoVector(const Job& job);

	/**
	 * \brief Loads chunks into device buffer
	 * \param nAccumulators number of accumulators to load for
	 * \param nChunks number of chunks to load
	 * \param startIdx starting chunk index in the file
	 * \param bytesProcessedPerAccumulator offset for each accumulator in bytes
	 * \param totalBytesPerAccumulator total bytes per accumulator
	 * \param buffer device buffer
	 * \param commandQueue command queue for the device
	 */
	void loadChunksIntoDeviceBuffer(
		size_t nAccumulators,
		size_t nChunks,
		size_t startIdx,
		size_t bytesProcessedPerAccumulator,
		size_t totalBytesPerAccumulator,
		const cl::Buffer& buffer,
		const cl::CommandQueue& commandQueue
	);

};
