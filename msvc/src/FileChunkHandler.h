#pragma once
#include <vector>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

constexpr size_t DEFAULT_CHUNK_SIZE = 4096;

/**
 * \brief Used to store state of the file - which chunks are left to be processed
 */
class FileChunkHandler {

	/**
	 * \brief Chunks are stored in a permutation where id represents id * chunkSize offset from the beginning of the file
	 *		  The vector actually serves as a stack which is iteratively popped in getNextNChunks method
	 *		  id is a 32 bit unsigned integer - which should be enough for reasonable chunk size, i.e. 4k
	 */
	std::vector<uint32_t> chunkPermutation;
	size_t chunkSize;
	size_t fileSize; // Size of the file
	fs::path filePath; // Path to the file

public:
	/**
	 * \brief Default ctor
	 * \param filePath path to the file - this must be validated before it is passed to the object
	 * \param percentRequired percent of the file to process - must be between (0, 1>
	 * \param chunkSize size of each chunk - default is 4k (DEFAULT_CHUNK_SIZE)
	 */
	explicit FileChunkHandler(const fs::path& filePath, double percentRequired = 1.0,
	                          const size_t chunkSize = DEFAULT_CHUNK_SIZE);

	inline auto getNextNChunks(const size_t n) {
		auto result = std::vector<uint32_t>();
		result.reserve(n); // reserve up to n items

		for (size_t i = 0; i < n; i += 1) {
			if (chunkPermutation.empty()) {
				// If there are no chunks left to process just return the result
				return result;
			}

			// Add popped item from the vector
			result.push_back(chunkPermutation.back());
			chunkPermutation.pop_back();
		}

		return result;
	}

	inline auto getFilePath() {
		return filePath;
	}
};
