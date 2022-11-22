#include "ClDeviceCoordinator.h"
#include <fstream>

void ClDeviceCoordinator::onProcessJob() {
	const auto maxHostChunks = static_cast<size_t>(floor(memoryLimit / chunkSizeBytes)) * chunkSizeBytes;
	auto readBuffer = std::vector<double>();
	readBuffer.reserve(maxHostChunks); // allocate with maxHostChunks

	// Open the file
	auto file = std::ifstream(distFilePath, std::ios::binary);

	// Get actual number of chunks
	auto [start, end] = currentJob->ChunkIdxRange;

	// Move to correct address in the file
	file.seekg(start * chunkSizeBytes * sizeof(double), std::ios::beg);  // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)

	// Create device buffer
	const auto deviceBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, maxNumberOfChunks * chunkSizeBytes);

	// Now simply run while loop until we have chunks to load to the CL device
	size_t chunksRemaining = end - start;
	size_t currentChunk = 0;
	while (chunksRemaining > 0) {
		const auto chunksToRead = chunksRemaining < maxHostChunks ? chunksRemaining : maxHostChunks;

		// Write to the device buffer
		commandQueue.enqueueWriteBuffer(deviceBuffer, CL_TRUE, currentChunk, readBuffer.capacity(),
		                                readBuffer.data());

		// Update
		currentChunk += chunksToRead;
		chunksRemaining -= chunksToRead;
	}

	// No point keeping the data
	readBuffer.clear();
	
}
