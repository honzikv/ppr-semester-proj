#pragma once
#include <filesystem>

namespace fs = std::filesystem;

namespace FilesystemUtils {

	/**
	 * \brief Creates directories for given file path, does nothing if such directories already exist
	 */
	void makeDirs(const fs::path& filePath) {
		const auto dirPath = filePath.parent_path();
		if (fs::is_regular_file(dirPath)) {
			return; // Nothing to do
		}

		if (!fs::create_directories(dirPath) && !fs::exists(dirPath)) {
			throw std::runtime_error("Failed to create directories for path: \"" + filePath.string() + "\"");
		}
		
	}
}
