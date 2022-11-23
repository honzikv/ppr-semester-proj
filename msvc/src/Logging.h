#pragma once
#include <mutex>
#include <vector>

enum Severity {
	DEBUG = 0,
	INFO = 1,
	CRITICAL = 2,
};

/**
 * \brief Very simple logging wrapper
 */
namespace Log {

	inline auto mutex = std::mutex();  // Mutex for thread safety
	inline auto severityLut = std::vector<std::string>{ "DEBUG", "INFO", "CRITICAL" };

}
