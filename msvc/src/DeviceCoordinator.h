#pragma once
#include <condition_variable>
#include <functional>
#include <iostream>

#include "Arghandling.h"
#include "Job.h"
#include "ConcurrencyUtils.h"
#include "DataLoader.h"

enum CoordinatorType {
	TBB = 0,
	OPEN_CL = 1,
	SINGLE_CORE = 2,
};

const auto COORDINATOR_TYPE_LUT = std::vector<std::string>{"TBB", "OPEN_CL", "SINGLE_CORE"};

namespace fs = std::filesystem;

/**
 * \brief This class is responsible for coordinating work on specified device - i.e. a GPU or SMP
 *		  This is also used as an abstraction over single thread execution
 *		  This class is a base class that is extended by SingleThreadCoordinator, SmpCoordinator and OpenClCoordinator
 */
class DeviceCoordinator {

protected:
	std::function<void(std::unique_ptr<Job>, size_t)> jobFinishedCallback;
	// Function called after job is processed - wakes up JobScheduler

	bool isActive = true; // Whether this coordinator is actually used (e.g. CPU is not used in OPENCL only mode)
	std::atomic<bool> isAvailable = true; // Whether the worker is available
	std::unique_ptr<Job> currentJob = nullptr; // Reference to the current job

	std::mutex jobMutex; // Mutex for assigning job

	/**
	 * \brief Semaphore used to synchronize access with JobScheduler
	 */
	std::shared_ptr<ConcurrencyUtils::Semaphore> semaphore = std::make_shared<ConcurrencyUtils::Semaphore>(0);
	std::atomic<bool> keepRunning = true; // Whether the coordinator thread should terminate
	std::thread coordinatorThread; // Thread that is responsible for processing jobs
	
	size_t memoryLimit; // max amount of memory this coordinator can allocate to buffer
	size_t maxNumberOfChunks{}; // This needs to be initialized in derived classes

	DataLoader dataLoader; // Data loader instance

	CoordinatorType coordinatorType;
	size_t id; // id of this coordinator
	
public:
	virtual ~DeviceCoordinator() = default;

	DeviceCoordinator(const CoordinatorType coordinatorType,
	                         const ProcessingMode processingMode,
	                         std::function<void(std::unique_ptr<Job>, size_t)> jobFinishedCallback,
	                         const size_t memoryLimit,
	                         const size_t chunkSizeBytes,
	                         fs::path& distFilePath,
	                         const size_t id):
		jobFinishedCallback(std::move(jobFinishedCallback)),
		memoryLimit(memoryLimit),
		dataLoader(distFilePath, chunkSizeBytes),
		coordinatorType(coordinatorType),
		id(id) {

		// Depending on the processing mode CPU coordinator may not be used and thus we don't want to create
		// an unnecessary thread - i.e. we check the coordinator type and processing mode, if they are
		// only OPENCL devices used we do not create the thread and set this to "dummy" state that will always
		// return available true but active false
		if ((coordinatorType == CoordinatorType::SINGLE_CORE || coordinatorType == CoordinatorType::TBB) &&
			processingMode == ProcessingMode::OPENCL_DEVICES) {
			isActive = false;
			return;
		}
	}

	auto getInfo() const {
		return std::string{COORDINATOR_TYPE_LUT.at(coordinatorType)} + " Coordinator with id " + std::to_string(id);
	}

	void startCoordinatorThread() {
		this->coordinatorThread = std::thread(&DeviceCoordinator::threadMain, this);
		coordinatorThread.detach();
	}

	/**
	 * \brief Returns true whether this coordinator is available - i.e. whether it is not processing any job
	 * \return true if the coordinator is available, false otherwise
	 */
	[[nodiscard]] auto available() const {
		return isAvailable.load();
	}

	/**
	 * \brief Returns whether this coordinator is active - i.e. whether it is usable for processing
	 * \return true if the coordinator is active, false otherwise
	 */
	[[nodiscard]] auto active() const {
		return isActive;
	}

	/**
	 * \brief Assigns job to the coordinator
	 * \param job job to be assigned
	 * \return void
	 */
	auto assignJob(Job job) {
		auto scopedLock = std::scoped_lock(jobMutex);
		isAvailable = false;
		currentJob = std::make_unique<Job>(std::move(job));
		semaphore->release();
	}

	/**
	 * \brief Terminates running thread by setting keepRunning to false
	 */
	void terminate() {
		keepRunning = false;
		semaphore->release();
	}

	/**
	 * \brief Returns maximum number of chunks that can be processed by this coordinator in a single job.
	 * This differs per device - i.e. GPU might be able to process more chunks than SMP
	 * \return maximum number of chunks
	 */
	virtual size_t getMaxNumberOfChunks() {
		return maxNumberOfChunks;
	}

private:
	/**
	 * \brief Main function of the coordinator thread
	 */
	void threadMain() {
		while (keepRunning) {
			semaphore->acquire();
			if (!currentJob) {
				continue;
			}
			
			processJob();
		}

	}

	/**
	 * \brief Processes given job - calls onProcessJob implementation and then calls jobFinishedCallback
	 */
	void processJob() {
		onProcessJob();
		auto scopedLock = std::scoped_lock(jobMutex);
		jobFinishedCallback(std::move(currentJob), id);
		isAvailable = true;
	}

protected:
	/**
	 * \brief This method is overriden by given implementation and called in processJob method
	 *		  Implementation must ensure that Watchdog is periodically notified - e.g. after each operation
	 */
	virtual void onProcessJob() = 0;

};
