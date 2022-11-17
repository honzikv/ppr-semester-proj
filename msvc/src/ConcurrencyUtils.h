#pragma once
#include <string>
#include <algorithm>
#include <mutex>

namespace Utils {
	template <typename T>
	auto lowercase(const std::basic_string<T>& s) {
		std::basic_string<T> s2 = s;
		std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
		return s2;
	}

	/**
	 * Returns minimum value of a and b
	 */
	template <typename T>
	auto getMin(T a, T b) {
		return a < b ? a : b;
	}

	class Semaphore {
		const size_t nPermissions;
		size_t nAvailable;
		std::mutex mutex;
		std::condition_variable conditionVariable;
	public:
		/** Default constructor. Default semaphore is a binary semaphore **/
		explicit Semaphore(const size_t& num_permissions = 1) : nPermissions(num_permissions),
		                                                        nAvailable(num_permissions) {
		}

		/** Copy constructor. Does not copy state of mutex or condition variable,
			only the number of permissions and number of available permissions **/
		Semaphore(const Semaphore& s) : nPermissions(s.nPermissions), nAvailable(s.nAvailable) {
		}

		void acquire() {
			std::unique_lock<std::mutex> lock(mutex);
			conditionVariable.wait(lock, [this] { return nAvailable > 0; });
			nAvailable -= 1;
			lock.unlock();
		}

		void release() {
			mutex.lock();
			nAvailable++;
			mutex.unlock();
			conditionVariable.notify_one();
		}
		
	};
}
