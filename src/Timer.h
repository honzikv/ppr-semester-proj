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
	Timer();
	~Timer();

	/**
	 * \brief Starts the timer
	 */
	void start() {
		startTimePoint = std::chrono::steady_clock::now();
	}

	/**
	 * \brief Stops the timer
	 */
	void stop() {
		endTimePoint = std::chrono::steady_clock::now();
	}

	/**
	 * \brief Returns the elapsed time in milliseconds
	 * \return Elapsed time in milliseconds
	 */
	[[nodiscard]] auto getElapsedTimeMillis() const {
		return std::chrono::duration_cast<std::chrono::milliseconds>(endTimePoint - startTimePoint);
	}

	auto printResults() const {
		const auto elapsedTime = getElapsedTimeMillis().count();

		// Print time in ns, ms and seconds
		std::cout << "Execution took " << StatUtils::doubleToStr(
			static_cast<double>(elapsedTime) / 1000.0, 5) << " s (" << std::to_string(elapsedTime) << " ms)" << std::endl;
	}

};

inline Timer::Timer() {
}

inline Timer::~Timer() {
}
