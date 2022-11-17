#pragma once
#include <condition_variable>
#include <functional>

#include "Job.h"
#include "ConcurrencyUtils.h"

enum CoordinatorType {
	TBB,
	OPEN_CL,
	SINGLE_CORE,
};

namespace Fs = std::filesystem;

/**
 * \brief This class is responsible for coordinating work on specified device - i.e. a GPU or SMP
 *		  This is also used as an abstraction over single thread execution
 *		  This class is a base class that is extended by SingleThreadCoordinator, SmpCoordinator and OpenClCoordinator
 */
class DeviceCoordinator {

protected:
	CoordinatorType coordinatorType;
	std::function<void(Job)> jobFinishedCallback; // Function called after job is processed - wakes up JobScheduler

	std::atomic<bool> isAvailable = true; // Whether the worker is available
	std::unique_ptr<Job> currentJob = nullptr; // Reference to the current job

	/**
	 * \brief Semaphore used to synchronize access with JobScheduler
	 */
	Utils::Semaphore semaphore = Utils::Semaphore();

	std::atomic<bool> keepRunning = true; // Whether the coordinator thread should terminate

	size_t chunkSize;
	size_t memoryLimit;
	uint32_t maxNumberOfChunks;  // This needs to be initialized in derived classes

	Fs::path& distFilePath;
	

public:
	virtual ~DeviceCoordinator() = default;

	inline DeviceCoordinator(const CoordinatorType coordinatorType, std::function<void(Job)> jobFinishedCallback,
	                         const size_t memoryLimit, const size_t chunkSize, Fs::path& distFilePath):
		coordinatorType(coordinatorType),
		jobFinishedCallback(std::move(jobFinishedCallback)),
		chunkSize(chunkSize),
		memoryLimit(memoryLimit),
		distFilePath(distFilePath) {
	}

	/**
	 * \brief Main function of the coordinator thread
	 */
	inline void coordinatorMain() {
		while (keepRunning) {
			// Try acquiring semaphore - this will be locked by default
			// and the thread has to wait until it is notified by the scheduler
			semaphore.acquire();
			if (!currentJob) {
				// If there is no current job just sleep
				// TODO this might be pointless
				continue;
			}

			processJob();
		}
	}

	[[nodiscard]] inline auto available() const {
		return isAvailable.load();
	}

	inline auto assignJob(Job job) {
		isAvailable = false;
		currentJob = std::make_unique<Job>(std::move(job));
		semaphore.release();
	}

	inline void terminate() {
		keepRunning = false;
		semaphore.release();
	}

	/**
	 * \brief Returns maximum number of chunks that can be processed by this coordinator in a single job.
	 * This differs per device - i.e. GPU might be able to process more chunks than SMP
	 * \return maximum number of chunks
	 */
	uint32_t getMaxNumberOfChunks() {
		return maxNumberOfChunks;
	}

protected:
	/**
	 * \brief Processes given job - calls onProcessJob implementation and then calls jobFinishedCallback
	 */
	inline void processJob() {
		onProcessJob();
		jobFinishedCallback(std::move(*currentJob));
		isAvailable = true; // TODO possible race condition?
	}

	/**
	 * \brief This method is overriden by given implementation and called in processJob method
	 *		  Implementation must ensure that Watchdog is periodically notified - e.g. after each operation
	 */
	virtual void onProcessJob() = 0;
	
};
