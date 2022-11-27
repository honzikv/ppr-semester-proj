#pragma once
#include <atomic>
#include <chrono>
#include <thread>

#include "Logging.h"

constexpr auto DEFAULT_SLEEP_MS = std::chrono::milliseconds{5000}; // 5s

class Watchdog {
	std::thread watchdogThread;
	std::atomic<size_t> counter;

	ConcurrencyUtils::Semaphore startSemaphore = ConcurrencyUtils::Semaphore(0);

	std::atomic<bool> keepRunning = true;
	std::chrono::milliseconds sleepMs{};

public:

	/**
	 * \brief Creates new Watchdog instance
	 * \param sleepMs amount of MS to sleep between checks
	 */
	explicit Watchdog(const std::chrono::milliseconds sleepMs = DEFAULT_SLEEP_MS): sleepMs(sleepMs) {
		// Start the thread
		watchdogThread = std::thread(&Watchdog::watchdogMain, this);
		watchdogThread.detach();
	}

	/**
	 * \brief Updates Watchdog's counter with value x - signals how many bytes were processed
	 * \param xBytes value to update counter with
	 */
	void updateCounter(const size_t xBytes) {
		counter += xBytes;
	}

	/**
	 * \brief Terminates the watchdog thread
	 */
	void terminate() {
		keepRunning = false;
	}

	/**
	 * \brief Starts the watchdog - i.e. releases the startSemaphore
	 */
	void start() {
		startSemaphore.release();
	}

private:
	void watchdogMain() {
		startSemaphore.acquire(); // Try to acquire start semaphore
		log(INFO, "[WATCHDOG] Watchdog is running");
		while (keepRunning) {
			std::this_thread::sleep_for(sleepMs);
			// Null the counter and take out the previous value for comparison
			const auto counterVal = counter.exchange(0);
			if (counterVal <= 0 && keepRunning) {
				log(CRITICAL, "[WATCHDOG] No progress in the last" + std::to_string(sleepMs.count()) + " ms");
				continue;
			}

			// Otherwise we can log the value
			if (keepRunning) {
				const auto kbsProcessed = counterVal / 1024;
				const auto mbsProcessed = kbsProcessed / 1024;
				log(INFO, "[WATCHDOG] Processed " + std::to_string(kbsProcessed) + " kBs (" + std::to_string(mbsProcessed) + " MBs) since last update.");
			}
		}
	}
};
