#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "Logging.h"

constexpr auto DEFAULT_SLEEP_MS = std::chrono::milliseconds{50000}; // 5s

class Watchdog {
	std::thread watchdogThread;
	std::atomic<size_t> counter;

	ConcurrencyUtils::Semaphore startSemaphore = ConcurrencyUtils::Semaphore(0);

	std::atomic<bool> keepRunning = true;
	std::chrono::milliseconds sleepMs;

public:
	explicit inline Watchdog(const std::chrono::milliseconds sleepMs = DEFAULT_SLEEP_MS): sleepMs(sleepMs) {
		// Start the thread
		watchdogThread = std::thread(&Watchdog::watchdogMain, this);
		watchdogThread.detach();
	}

	/**
	 * \brief Updates Watchdog's counter with value x
	 * \param x value to update counter with
	 */
	void updateCounter(const size_t x) {
		counter += x;
	}

	void terminate() {
		keepRunning = false;
	}

private:
	void watchdogMain() {
		startSemaphore.acquire(); // Try to acquire start semaphore
		while (keepRunning) {
			std::this_thread::sleep_for(sleepMs);
			const auto counterVal = counter.exchange(0);
			if (counterVal == 0 && keepRunning) {
				log(CRITICAL, "Watchdog: No progress in the last" + std::to_string(sleepMs.count()) + " ms" );
				continue;
			}

			if (counterVal > 0) {
				log(INFO, "Processed " + std::to_string(counterVal) + " chunks since last update.");
			}
			
		}
	}
};
