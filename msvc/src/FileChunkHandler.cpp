#include "FileChunkHandler.h"
#include <algorithm>
#include <numeric>

FileChunkHandler::FileChunkHandler(const fs::path& filePath, double percentRequired, const size_t chunkSize):
	chunkSize(chunkSize),
	fileSize(fs::file_size(filePath)),
	filePath(filePath) {
	const auto maxChunks = static_cast<size_t>(std::ceil(
		static_cast<double>(fileSize) / static_cast<double>(chunkSize)));

	const auto totalChunks = static_cast<size_t>(std::ceil(
		static_cast<double>(maxChunks) * percentRequired));

	// Generate permutation of chunks
	chunkPermutation.resize(maxChunks);
	std::iota(chunkPermutation.begin(), chunkPermutation.end(), 0);

	// Shuffle the permutation
	std::shuffle(chunkPermutation.begin(), chunkPermutation.end(), std::random_device());

	// chunkPermutation = chunkPermutation[:totalChunks]
	chunkPermutation = std::vector(chunkPermutation.begin(), chunkPermutation.begin() + totalChunks);
}
