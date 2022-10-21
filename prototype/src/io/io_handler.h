#pragma once

#include <filesystem>
#include <fstream>
#include <numeric>
#include <random>
#include <mutex>

namespace fs = std::filesystem;

constexpr uint16_t DEFAULT_CHUNK_SIZE = 4096;  // 4 kB

class IOHandler {
private:

    const uint32_t chunkSize = 0;

    size_t chunksProcessed = 0;

    size_t chunksTotal = 0;

    size_t chunkPermutationIdx = 0;

    std::mutex mutex;

    std::vector<uint32_t> chunkPermutation;

    std::ifstream fileStream;

public:
    inline explicit IOHandler(const fs::path& filePath, uint32_t chunkSize = DEFAULT_CHUNK_SIZE) : fileStream(
            filePath.string()), chunkSize(chunkSize) {
        // Check if file exists
        if (!fileStream) {
            throw std::runtime_error("File does not exist");
        }

        // Calculate size
        auto fileSize = fs::file_size(filePath);

        // Calculate number of chunks
        chunksTotal = static_cast<uint32_t>(ceil(fileSize / chunkSize));

        // Create permutation for chunks
        std::iota(chunkPermutation.begin(), chunkPermutation.end(), 0);
        std::shuffle(chunkPermutation.begin(), chunkPermutation.end(), std::mt19937(std::random_device{}()));
    }

    inline ~IOHandler() {
        fileStream.close();
    }

    inline std::vector<double> readChunks(const std::vector<uint32_t>& chunks) {
        auto result = std::vector<double>(chunks.size() * chunkSize);

        auto lock = std::scoped_lock(mutex);
        for (const auto& chunk: chunks) {
            // Move to the position of the chunk
            fileStream.seekg(chunk * chunkSize);

            // Read at current index * DEFAULT_CHUNK_SIZE
            auto idx = &chunk - &chunks[0];
            fileStream.read(reinterpret_cast<char*>(&result[idx * chunkSize]), chunkSize);
        }

        return result;
    }

    /**
     * Returns next N chunk ids
     * \param n
     * \return next N chunk ids
     */
    inline std::vector<uint32_t> getNextNChunkIds(uint32_t n) {
        auto result = std::vector<uint32_t>();
        result.reserve(n);
        for (auto i = 0; i < n || chunkPermutationIdx >= chunksTotal; i += 1) {
            // Get next chunk
            auto chunk = chunkPermutation[chunkPermutationIdx];

            // Add chunk to result
            result.push_back(chunk);

            // Increase chunk permutation index
            chunkPermutationIdx += 1;
        }

        return result;
    }

    /**
     * Returns true if file is processed, false otherwise
     * \return
     */
    [[nodiscard]] inline bool fileProcessed() const {
        return chunkPermutationIdx < chunksTotal;
    }


};
