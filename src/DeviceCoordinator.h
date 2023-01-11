#pragma once
#include <functional>

#include "ProcessingConfig.h"
#include "Job.h"
#include "ConcurrencyUtils.h"
#include "DataLoader.h"
#include "Logging.h"

enum CoordinatorType {
	TBB = 0,
	OPEN_CL = 1,
	SINGLE_CORE = 2,
};

const auto COORDINATOR_TYPE_LUT = std::vector<std::string>{"SMP", "OPEN_CL", "SINGLE_CORE"};

namespace fs = std::filesystem;

/**
 * \brief Coordinator error that occurs during execution - i.e. Kernel fails to compile
 */
struct CoordinatorErr {

	size_t JobId;
	size_t CoordinatorId;
	std::string What;
	bool IsFatal;

	/**
	 * \brief Default constructor for CoordinatorErr
	 * \param jobId id of the job where the error occurred
	 * \param what what caused the error
	 * \param coordinatorId index of the coordinator processing the job
	 * \param fatal whether it is fatal - i.e. requires termination of the program
	 */
	CoordinatorErr(const size_t jobId, std::string what, const size_t coordinatorId, const bool fatal = true):
		JobId(jobId),
		CoordinatorId(coordinatorId),
		What(std::move(what)),
		IsFatal(fatal) {
	}
};

/**
 * \brief This abstract class is responsible for coordinating work on specified device - i.e. a GPU or SMP
 */
class DeviceCoordinator {

protected:
	/**
	 * \brief JobFinishedCallback is a function which is called upon finishing a job. This way we do not have circular
	 *		  dependency between JobScheduler and DeviceCoordinator
	 */
	std::function<void(std::unique_ptr<Job>, size_t)> jobFinishedCallback;

	/**
	 * \brief Callback function for notifying Watchdog
	 */
	std::function<void(size_t)> notifyWatchdogCallback;

	/**
	 * \brief Callback for error handling
	 */
	std::function<void(CoordinatorErr)> errCallback;

	bool isEnabled = true;
	std::unique_ptr<Job> currentJob = nullptr; // Reference to the current job

	std::mutex jobMutex; // Mutex for assigning job

	/**
	 * \brief Semaphore used to synchronize access with JobScheduler
	 */
	std::shared_ptr<ConcurrencyUtils::Semaphore> semaphore = std::make_shared<ConcurrencyUtils::Semaphore>(0);
	std::atomic<bool> keepRunning = true; // Whether the coordinator thread should terminate
	std::thread coordinatorThread; // Thread that is responsible for processing jobs

	/**
	 * \brief Size of chunk in bytes
	 */
	size_t chunkSizeBytes;

	/**
	 * \brief Max number of chunks that can be processed within a single job assigned by JobScheduler.
	 *		  For OpenCL device this will be more than the host memory since data is loaded into the device memory
	 */
	size_t maxNumberOfChunksPerJob{};


	/**
	 * \brief Number of bytes per StatsAccumulator object - this dictates how many StatsAccumulator objects are computed
	 */
	size_t bytesPerAccumulator;

	/**
	 * \brief Path to the file that is being processed
	 */
	fs::path& filePath;

	/**
	 * \brief Type of the coordinator - mostly used for debugging
	 */
	CoordinatorType coordinatorType;

	/**
	 * \brief Identifier of the coordinator - mostly used for debugging
	 */
	size_t id;

	DataLoader dataLoader;


public:
	virtual ~DeviceCoordinator();

	/**
	 * \brief Creates new device coordinator. This should must be called by derived classes
	 * \param coordinatorType type of the coordinator
	 * \param processingMode processing mode
	 * \param jobFinishedCallback callback after job is finished
	 * \param notifyWatchdogCallback callback for notifying watchdog
	 * \param errCallback error callback for error handling
	 * \param chunkSizeBytes chunk size in bytes
	 * \param bytesPerAccumulator number of bytes processed by each StatsAccumulator
	 * \param distFilePath path to the file that is being processed
	 * \param id id of this device coordinator
	 */
	DeviceCoordinator(const CoordinatorType coordinatorType,
	                  const ProcessingMode processingMode,
	                  std::function<void(std::unique_ptr<Job>, size_t)> jobFinishedCallback,
	                  std::function<void(size_t)> notifyWatchdogCallback,
	                  std::function<void(CoordinatorErr)> errCallback,
	                  const size_t chunkSizeBytes,
	                  const size_t bytesPerAccumulator,
	                  fs::path& distFilePath,
	                  const size_t id):
		jobFinishedCallback(std::move(jobFinishedCallback)),
		notifyWatchdogCallback(std::move(notifyWatchdogCallback)),
		errCallback(std::move(errCallback)),
		chunkSizeBytes(chunkSizeBytes),
		bytesPerAccumulator(bytesPerAccumulator),
		filePath(distFilePath),
		coordinatorType(coordinatorType),
		id(id),
		dataLoader(filePath, chunkSizeBytes) {

		// Depending on the processing mode CPU coordinator may not be used and thus we don't want to create
		// an unnecessary thread - i.e. we check the coordinator type and processing mode, if they are
		// only OPENCL devices used we do not create the thread and set this to "dummy" state that will always
		// return available true but enabled false
		if ((coordinatorType == CoordinatorType::SINGLE_CORE || coordinatorType == CoordinatorType::TBB) &&
			processingMode == ProcessingMode::OPENCL_DEVICES) {
			isEnabled = false;
			return;
		}
	}

	std::string getInfo() const {
		return std::string{COORDINATOR_TYPE_LUT.at(coordinatorType)} + " Coordinator with id " + std::to_string(id);
	}

	/**
	 * \brief Join the coordinator thread - this must be called to ensure the program is terminated correctly
	 */
	void join() {
		if (coordinatorThread.joinable()) {
			coordinatorThread.join();
		}
	}

	void startCoordinatorThread() {
		coordinatorThread = std::thread(&DeviceCoordinator::threadMain, this);
	}

	/**
	 * \brief Returns whether this coordinator is active - i.e. whether it is usable for processing
	 * \return true if the coordinator is active, false otherwise
	 */
	[[nodiscard]] bool enabled() const {
		return isEnabled;
	}

	/**
	 * \brief Assigns job to the coordinator
	 * \param job job to be assigned
	 */
	void assignJob(Job job) {
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
		return maxNumberOfChunksPerJob;
	}

private:
	/**
	 * \brief Main function of the coordinator thread
	 */
	void threadMain() {
		log(DEBUG, "[DEVICECOORDINATOR] Coordinator " + std::to_string(id) + "'s thread started...");
		while (keepRunning) {
			semaphore->acquire();
			if (!currentJob) {
				continue;
			}

			try {
				processJob();
			}
			catch (const std::runtime_error& err) {
				errCallback({currentJob->Id, err.what(), id});
			}
		}
	}

	/**
	 * \brief Processes given job - calls onProcessJob implementation and then calls jobFinishedCallback
	 */
	void processJob() {
		onProcessJob();
		jobFinishedCallback(std::move(currentJob), id);
	}

protected:
	/**
	 * \brief This method is overriden by given implementation and called in processJob method
	 *		  Implementation must ensure that Watchdog is periodically notified - e.g. after each operation
	 */
	virtual void onProcessJob();

};

inline DeviceCoordinator::~DeviceCoordinator() = default;

inline void DeviceCoordinator::onProcessJob() {
}
