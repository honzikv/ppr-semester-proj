#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

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
			if (counter == 0) {
				std::cout << "No job detected ..." << std::endl;
				continue;
			}

			counter = 0;
		}
	}
};
