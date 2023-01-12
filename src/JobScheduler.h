#pragma once
#include "CpuDeviceCoordinator.h"
#include "Avx2CpuDeviceCoordinator.h"
#include "ClDeviceCoordinator.h"
#include "FileChunkHandler.h"
#include "MemoryAllocation.h"
#include "Watchdog.h"

constexpr auto DEFAULT_CHUNK_SIZE = 4096;
constexpr auto SMALL_SIZE_LIMIT = 1024 * 1024; // 1 MB

constexpr auto DEFAULT_BYTES_PROCESSED_BY_ACCUMULATOR_CPU = 1ULL * 1024 * 1024 * sizeof(double);
constexpr auto DEFAULT_BYTES_PROCESSED_BY_ACCUMULATOR_CL = 1ULL * 1024 * 1024 * sizeof(double);

/**
 * \brief This class acts as a load balancer and job manager. It schedules job among available coordinators and accumulates results
 */
class JobScheduler {

	/**
	 * \brief List of all device coordinators
	 */
	std::vector<std::shared_ptr<ClDeviceCoordinator>> clDeviceCoordinators;

	/**
	 * \brief This vector stores availability of each device coordinator
	 */
	std::vector<bool> coordinatorAvailability;

	/**
	 * \brief Coordinator for CPU (SMP). This is shared_ptr to allow for polymorphism
	 */
	std::shared_ptr<CpuDeviceCoordinator> cpuDeviceCoordinator = nullptr;

	/**
	 * \brief To synchronize with device coordinators we use a semaphore which is incremented by coordinator after
	 * finishing a job
	 */
	ConcurrencyUtils::Semaphore jobFinishedSemaphore = ConcurrencyUtils::Semaphore(0);

	/**
	 * \brief Synchronization mutex for coordinators
	 */
	std::mutex coordinatorMutex;

	/**
	 * \brief File chunk handler instance - this is initialized later in the constructor, therefore it is wrapped in unique_ptr
	 */
	std::unique_ptr<FileChunkHandler> fileChunkHandler;

	/**
	 * \brief Instance of watch dog
	 */
	std::unique_ptr<Watchdog> watchdog;

	/**
	 * \brief Id of the "current" job - i.e. job we are currently assigning 
	 */
	size_t currentJobId = 0;

	/**
	 * \brief Accumulated results from the coordinators
	 */
	std::vector<StatsAccumulator> accumulators;

	/**
	 * \brief Last execution error, coordinators set this up via notifyErrOccurred callback
	 */
	std::unique_ptr<CoordinatorErr> lastErr = nullptr;

public:
	explicit JobScheduler(ProcessingConfig& processingConfig, size_t chunkSizeBytes = DEFAULT_CHUNK_SIZE);

	~JobScheduler();

	/**
	 * \brief Returns whether there is any job remaining
	 * \return true if there is any job remaining, false otherwise
	 */
	[[nodiscard]] bool jobRemaining() const {
		// Job is available when there are still some chunks left to process
		return !fileChunkHandler->allChunksProcessed();
	}

	/**
	 * \brief Returns if there is any coordinator available
	 * \return true if there is any coordinator available, false otherwise
	 */
	bool anyCoordinatorAvailable();

	/**
	 * \brief Returns if there is any coordinator processing job
	 * \return true if all coordinators are available, false otherwise
	 */
	bool allCoordinatorsAvailable();

	/**
	 * \brief Gets next best available coordinator. Caller must ensure that there is at least one coordinator available
	 * \return shared pointer to given coordinator - this is cast to DeviceCoordinator
	 */
	std::pair<size_t, std::shared_ptr<DeviceCoordinator>> getNextAvailableDeviceCoordinator();

	/**
	 * \brief Adds processed job to the accumulated results
	 * \param job unique pointer to the job
	 */
	void addProcessedJob(std::unique_ptr<Job> job);

	/**
	 * \brief Callback for DeviceCoordinator to notify that job is finished
	 * \param job processed job
	 * \param coordinatorIdx index of the coordinator
	 */
	void jobFinishedCallback(std::unique_ptr<Job> job, size_t coordinatorIdx);

	/**
	 * \brief Callback for device coordinator to kick the watchdog
	 * \param bytesProcessed number of bytes processed by the coordinator
	 */
	void notifyWatchdogCallback(size_t bytesProcessed);

	/**
	 * \brief Callback for device coordinator to notify that error occurred
	 * \param err error that occurred
	 */
	void notifyErrOccurred(const CoordinatorErr& err);

	/**
	 * \brief Terminates all device coordinator - sets flag to exit the thread loop which will eventually terminate
	 */
	void terminateDeviceCoordinators() const;

	/**
	 * \brief Assigns job to available coordinator
	 */
	void assignJob();

	/**
	 * \brief Checks for errors and throws an instance of std::runtime_error if any exception (that was fatal) occurred 
	 */
	void checkForErrors();

	/**
	 * \brief Runs the job scheduler.
	 */
	std::vector<StatsAccumulator> run();
};
