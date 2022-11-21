#include "ClDeviceCoordinator.h"

void ClDeviceCoordinator::onProcessJob()
{
    auto readBuffer = std::vector<double>(floor(memoryLimit / chunkSize) * chunkSize);

    // Open the file
    auto file = std::ifstream(distFilePath, std::ios::binary);

    // Get actual number of chunks
    auto [start, end] = currentJob->ChunkIdxRange;

    // Move to correct address in the file
    file.seekg(start * chunkSize * sizeof(double), std::ios::beg);

    // Create device buffer
    auto deviceBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, maxNumberOfChunks * chunkSize);

    // Read all bytes to the buffer on the device
    auto chunksRemaining = end - start;
    auto currentChunk = 0;
    while (true) {
        const auto chunksToRead = std::min(chunksRemaining, maxNumberOfChunks);
        file.read(reinterpret_cast<char *>(), chunksToRead * chunkSize);

        // Write to the device buffer
        commandQueue.enqueueWriteBuffer(deviceBuffer, CL_TRUE, currentChunk, readBuffer.capacity(),
                                        readBuffer.data());
        
        currentChunk += chunksToRead;
        chunksRemaining -= chunksToRead;
    }


}
