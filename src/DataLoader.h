#pragma once
#include <fstream>
#include <filesystem>
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

private:
	/**
	 * \brief Computes metadata for given job
	 * \param startIdx start index (inclusive)
	 * \param endIdx end index (exclusive)
	 * \return triplet of number of chunks, number of bytes to read, and starting address
	 */
	[[nodiscard]] std::tuple<size_t, size_t, size_t> computeChunkMetadata(const size_t startIdx, const size_t endIdx) const;

public:
	/**
	 * \brief Loads all job data into buffer and returns it
	 * \param job job
	 * \return vector of doubles containing all loaded job data
	 */
	std::vector<double> loadJobDataIntoVector(const Job& job);


	/**
	 * \brief Loads chunks into OpenCL device's buffer
	 * \param startIdx start index of the chunk in file
	 * \param endIdx end index of the chunk in file
	 * \param buffer buffer to write the data to
	 * \param commandQueue command queue to execute enqueueWriteBuffer on
	 */
	void loadChunksIntoDeviceBuffer(size_t startIdx, size_t endIdx, const cl::Buffer& buffer,
	                                const cl::CommandQueue& commandQueue);

};
