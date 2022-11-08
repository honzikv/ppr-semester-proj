#pragma once
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <random>

namespace fs = std::filesystem;

constexpr auto DEFAULT_CHUNK_SIZE = 4096;

/**
 * \brief Structure serving as an abstraction to the processed file
 */
class IoHandler {

	std::vector<bool> unprocessedChunks;
	std::vector<uint32_t> chunkPermutation; // Permutation of chunks to ensure random data
	std::ifstream fileStream;
	size_t fileSize;

public:
	IoHandler(const fs::path& filePath, const size_t chunkSize = DEFAULT_CHUNK_SIZE):
		fileStream(filePath, std::ios::binary) {
		fileSize = fs::file_size(filePath);

		// Compute total number of chunks
		const auto totalChunks = fileSize / chunkSize + (fileSize % chunkSize != 0);

		// Resize unprocessed chunks bitmap
		unprocessedChunks.resize(totalChunks, true);

		// Generate permutation of chunks
		chunkPermutation.resize(totalChunks);
		std::iota(chunkPermutation.begin(), chunkPermutation.end(), 0);

		// Shuffle the permutation
		std::shuffle(chunkPermutation.begin(), chunkPermutation.end(), std::random_device());
	}
};
