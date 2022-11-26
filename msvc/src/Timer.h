#pragma once
#include <chrono>

#include "Logging.h"

/**
 * \brief Simple class for measuring execution time
 */
class Timer {

	std::chrono::time_point<std::chrono::steady_clock> startTimePoint;
	std::chrono::time_point<std::chrono::steady_clock> endTimePoint;


public:
	Timer() = default;
	~Timer() = default;

	/**
	 * \brief Starts the timer
	 */
	void start() {
		startTimePoint = std::chrono::high_resolution_clock::now();
	}

	/**
	 * \brief Stops the timer
	 */
	void stop() {
		endTimePoint = std::chrono::high_resolution_clock::now();
	}


	/**
	 * \brief Returns the elapsed time in milliseconds
	 * \return Elapsed time in milliseconds
	 */
	auto getElapsedTimeNanoSecs() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(endTimePoint - startTimePoint).count();
	}

	auto logResults(LogSeverity logSeverity = INFO) {
		const auto elapsedTime = getElapsedTimeNanoSecs();
		// Print time in ns, ms and seconds
		log(logSeverity, "Elapsed time: " + std::to_string(elapsedTime) + " ns = "
		    + std::to_string(elapsedTime / 1000000) + " ms = " + std::to_string(
			    elapsedTime / 1000000000)
		    + " s)");
	}

};
