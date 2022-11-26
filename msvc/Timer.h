#pragma once
#include <chrono>

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
		return std::chrono::duration_cast<std::chrono::nanoseconds>(startTimePoint - endTimePoint);
	}

};
