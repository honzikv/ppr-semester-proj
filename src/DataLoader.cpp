#include "DataLoader.h"

DataLoader::DataLoader(const fs::path& filePath, const size_t chunkSizeBytes) : ChunkSizeBytes(chunkSizeBytes) {
	file.open(filePath, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Unable to open file: " + filePath.string());
	}
}

std::vector<double> DataLoader::loadJobDataIntoVector(const Job& job) {
	const auto [startIdx, endIdx] = job.ChunkIdxRange;
	const auto nChunks = endIdx - startIdx;
	const auto bytesToRead = nChunks * ChunkSizeBytes;
	const auto address = startIdx * ChunkSizeBytes;

	// Create memory buffer, note that we assume that chunkSizeBytes is a multiple of sizeof(double)
	auto buffer = std::vector<double>(bytesToRead / sizeof(double));
	if (buffer.empty()) {
		return buffer;
	}

	// Move to correct address in the file
	file.seekg(static_cast<int64_t>(address), std::ios::beg);

	// Read data into the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), static_cast<int64_t>(bytesToRead));

	// Return the buffer
	return buffer;
}

void DataLoader::loadChunksIntoDeviceBuffer(
	const size_t nAccumulators,
	const size_t nChunks,
	const size_t startIdx,
	const size_t bytesProcessedPerAccumulator,
	const size_t totalBytesPerAccumulator,
	const cl::Buffer& buffer,
	const cl::CommandQueue& commandQueue
) {

	const auto bytesToRead = nChunks * ChunkSizeBytes;

	// Create host buffer
	auto hostBuffer = std::vector<double>(bytesToRead / sizeof(double));

	const auto bytesPerAccumulator = bytesToRead / nAccumulators;

	// Iterate over each accumulator and load data for processing
	for (auto accumulatorId = 0ULL; accumulatorId < nAccumulators; accumulatorId += 1) {

		// Compute address for the current accumulator - i.e. start position in the file + accumulatorId * accumulator offset + 
		const auto address = startIdx * ChunkSizeBytes + accumulatorId * totalBytesPerAccumulator +
			bytesProcessedPerAccumulator;

		// Move to the address
		file.seekg(static_cast<int64_t>(address), std::ios::beg);

		// Read bytes to the buffer
		file.read(reinterpret_cast<char*>(hostBuffer.data() + accumulatorId * bytesPerAccumulator / sizeof(double)),
		          static_cast<int64_t>(bytesPerAccumulator));
	}

	if (const auto returnValue = commandQueue.enqueueWriteBuffer(buffer, CL_TRUE, 0, nChunks * ChunkSizeBytes,
	                                                             hostBuffer.data());
		returnValue != CL_SUCCESS) {
		throw std::runtime_error("Could not allocate memory on the device, the program cannot continue!");
	}
}
