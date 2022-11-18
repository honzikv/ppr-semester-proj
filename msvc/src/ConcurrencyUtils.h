#pragma once
#include <string>
#include <algorithm>
#include <mutex>

namespace ConcurrencyUtils {
	// class Semaphore {
	// 	const size_t nPermissions;
	// 	size_t nAvailable;
	// 	std::mutex mutex;
	// 	std::condition_variable conditionVariable;
	//
	// public:
	// 	/** Default constructor. Default semaphore is a binary semaphore **/
	// 	explicit Semaphore(const size_t& numPermissions = 1) : nPermissions(numPermissions),
	// 	                                                       nAvailable(numPermissions) {
	// 	}
	//
	// 	/** Copy constructor. Does not copy state of mutex or condition variable,
	// 		only the number of permissions and number of available permissions **/
		// Semaphore(const Semaphore& s) : nPermissions(s.nPermissions), nAvailable(s.nAvailable) {
		// }
	//
	// 	void acquire() {
	// 		std::unique_lock<std::mutex> lock(mutex);
	// 		conditionVariable.wait(lock, [this] { return nAvailable > 0; });
	// 		nAvailable -= 1;
	// 		lock.unlock();
	// 	}
	//
	// 	void release() {
	// 		mutex.lock();
	// 		nAvailable += 1;
	// 		mutex.unlock();
	// 		conditionVariable.notify_one();
	// 	}
	//
	// };

	class Semaphore {
		std::mutex mutex;
		std::condition_variable conditionVariable;
		size_t count;

	
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
