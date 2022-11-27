#include "DataLoader.h"

DataLoader::DataLoader(const fs::path& filePath, const size_t chunkSizeBytes) : ChunkSizeBytes(chunkSizeBytes) {
	file.open(filePath, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Unable to open file: " + filePath.string());
	}
}

std::tuple<size_t, size_t, size_t> DataLoader::computeChunkMetadata(const size_t startIdx, const size_t endIdx) const {
	const auto nChunks = endIdx - startIdx;
	const auto bytesToRead = nChunks * ChunkSizeBytes;
	const auto address = startIdx * ChunkSizeBytes;

	return std::make_tuple(nChunks, bytesToRead, address);
}

std::vector<double> DataLoader::loadJobDataIntoVector(const Job& job) {
	const auto [startIdx, endIdx] = job.ChunkIdxRange;
	const auto [nChunks, bytesToRead, address] = computeChunkMetadata(startIdx, endIdx);

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

void DataLoader::loadChunksIntoDeviceBuffer(const size_t startIdx, const size_t endIdx, const cl::Buffer& buffer,
                                            const cl::CommandQueue& commandQueue) {
	const auto [nChunks, bytesToRead, address] = computeChunkMetadata(startIdx, endIdx);

	// Create host buffer
	auto hostBuffer = std::vector<double>(bytesToRead / sizeof(double));

	// Move to correct address in the file
	file.seekg(static_cast<int64_t>(address), std::ios::beg);

	// Read data into the buffer
	file.read(reinterpret_cast<char*>(hostBuffer.data()), static_cast<int64_t>(bytesToRead));

	// Copy host buffer to the device buffer
	if (const auto returnValue = commandQueue.enqueueWriteBuffer(buffer, CL_TRUE, 0, bytesToRead, hostBuffer.data());
		returnValue != CL_SUCCESS) {
		throw std::runtime_error("Could not allocate memory on the device, the program cannot continue!");
	}

}
