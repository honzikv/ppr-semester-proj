#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <thread>

class Watchdog {
	std::thread watchdogThread;
	std::atomic<uint32_t> counter;

	std::condition_variable startCondition;
	std::mutex mutex;

	std::atomic<bool> keepRunning = true;
	std::chrono::milliseconds sleepMs;

public:
	inline Watchdog(std::chrono::milliseconds sleepMs): sleepMs(sleepMs) {
		// Start the thread
		watchdogThread = std::thread(&Watchdog::watchdogMain, this);
		watchdogThread.detach();
	}

	void updateCounter(int x) {
		counter += x;
	}

private:
	inline void watchdogMain() {
		auto lock = std::unique_lock(mutex);
		startCondition.wait(lock);

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