#pragma once
#include <mutex>

namespace ConcurrencyUtils {

	/**
	 * \brief Basic implementation of counting semaphore
	 */
	class Semaphore {
		std::mutex mutex; // Mutex for locking
		std::condition_variable conditionVariable; // Condition variable for synchronization
		size_t count; // Number of available resources

	public:
		auto acquire() {
			auto lock = std::unique_lock(mutex);
			while (count == 0) {
				conditionVariable.wait(lock);
			}

			count -= 1;
		}

		auto release() {
			auto lock = std::scoped_lock(mutex);
			count += 1;
			conditionVariable.notify_one();
		}

		explicit Semaphore(const size_t count) : count(count) {
		}
	};
}
