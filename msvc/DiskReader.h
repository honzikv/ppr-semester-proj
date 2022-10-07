#pragma once
#include <bitset>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

constexpr auto BYTES_PER_TASK = 8 * 1024 * 1024; // 8 MB per task
constexpr auto CHUNK_SIZE = 1024; // 1 kB

struct Config {
	int NumWorkers;
	fs::path FilePath; // path to the file
};

/**
 * \brief This class is used for reading from disk - it holds path to the file and buffer from which the workers get their data
 */
class DiskReader {

private:
	/**
	 * \brief Buffer from which the workers get their data
	 */
	std::vector<char> buffer;
	std::vector<bool> processedChunks;

		Config config;

	explicit DiskReader(const Config& config) : buffer(config.NumWorkers * BYTES_PER_TASK), config(config) {
	}



};
