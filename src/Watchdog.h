#pragma once
#include <atomic>
#include <chrono>
#include <thread>

#include "Logging.h"

constexpr auto DEFAULT_SLEEP_MS = std::chrono::milliseconds{5000}; // 5s

class Watchdog {
	/**
	 * \brief Watchdog thread instance for join operation when the thread is to be destroyed
	 */
	std::thread watchdogThread;

	/**
	 * \brief Watchdog counter - this is periodically checked to ensure jobs are being processed successfully
	 */
	std::atomic<size_t> counter;

	/**
	 * \brief Semaphore for start synchronization
	 */
	ConcurrencyUtils::Semaphore startSemaphore = ConcurrencyUtils::Semaphore(0);

	/**
	 * \brief Flag to keep the thread running
	 */
	std::atomic<bool> keepRunning = true;

	/**
	 * \brief Sleep time in milliseconds
	 */
	std::chrono::milliseconds sleepMs{};

	/**
	 * \brief Mutex for synchronization
	 */
	std::mutex mutex;

	/**
	 * \brief Condition variable for sleep
	 */
	std::condition_variable sleepCondition;

public:
	/**
	 * \brief Creates new Watchdog instance
	 * \param sleepMs amount of MS to sleep between checks
	 */
	explicit Watchdog(const std::chrono::milliseconds sleepMs = DEFAULT_SLEEP_MS): sleepMs(sleepMs) {
		log(INFO, "[WATCHDOG] Watchdog created, timeout set to: " + std::to_string(sleepMs.count()) + " ms");
		// Start the thread
		watchdogThread = std::thread(&Watchdog::watchdogMain, this);
	}

	/**
	 * \brief Updates Watchdog's counter with value x - signals how many bytes were processed
	 * \param xBytes value to update counter with
	 */
	void updateCounter(const size_t xBytes) {
		counter += xBytes;
	}

	/**
	 * \brief Joins the Watchdog thread (if joinable)
	 */
	void join() {
		keepRunning = false;
		sleepCondition.notify_one();

		if (watchdogThread.joinable()) {
			watchdogThread.join();
		}
	}

	/**
	 * \brief Terminates the watchdog thread
	 */
	void terminate() {
		keepRunning = false;
		sleepCondition.notify_one();
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
		log(DEBUG, "[WATCHDOG] Watchdog is ready and running ...");
		while (keepRunning) {
			{
				auto uniqueLock = std::unique_lock(mutex);
				sleepCondition.wait_for(uniqueLock, sleepMs);
			}
			// Null the counter and take out the previous value for comparison
			const auto counterVal = counter.exchange(0);
			if (counterVal <= 0 && keepRunning) {
				log(WARNING, "[WATCHDOG] No progress detected in the last " + std::to_string(sleepMs.count()) + " ms");
				continue;
			}

			// Otherwise we can log the value
			if (keepRunning) {
				const auto kbsProcessed = counterVal / 1024;
				const auto mbsProcessed = kbsProcessed / 1024;
				log(INFO, "[WATCHDOG] Processed " + std::to_string(kbsProcessed) + " kB (" +
				    std::to_string(mbsProcessed) + " MB) since the last update.");
			}
		}
	}
};
