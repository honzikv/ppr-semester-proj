#pragma once

#include <condition_variable>
#include <thread>

/**
 * Implementation of watchdog
 */
class Watchdog {

public:

    /**
     * Creates a new watchdog instance
     * \param sleepMs time in milliseconds to sleep between checks
     */
    Watchdog(std::chrono::duration<size_t> sleepMs) {
        this->sleepMs = sleepMs;

        // Create the thread and
        watchdogThread = std::thread(&Watchdog::watchdogMain, this);
        watchdogThread.detach();
    }

    /**
     * Wakes the watchdog thread up to start monitoring the counter
     */
    inline void start() {
        startCondition.notify_one();
    }

    /**
     * Gracefully terminates the watchdog thread
     */
    inline void terminate() {
        terminateThread = true;
    }

private:
    /**
     * watchdog thread object
     */
    std::thread watchdogThread;

    /**
     * Sleep time between watchdog checks
     */
    std::chrono::duration<size_t> sleepMs;

    /**
     * Workers increment this counter to signal that they are still alive and how much work they have done
     */
    std::atomic<size_t> counter = 0;

    /**
     * Last value for the counter variable
     */
    size_t lastValue = 0;

    /**
     * Used to terminate the watchdog thred
     */
    std::atomic<bool> terminateThread = false;

    /**
     * This is used to prevent the watchdog from starting before the workers are ready
     */
    std::condition_variable startCondition;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mutex);

    /**
     * Main watchdog thread function
     */
    inline void watchdogMain() {
        startCondition.wait(lock); // Wait for JobManager to start the watchdog

        // Loop until the watchdog is terminated
        while (!terminateThread) {

            // Sleep for a while
            std::this_thread::sleep_for(sleepMs);

        }
    }

};
