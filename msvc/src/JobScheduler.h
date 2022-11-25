#pragma once
#include "CpuDeviceCoordinator.h"
#include "Arghandling.h"
#include "Avx2CpuDeviceCoordinator.h"
#include "ClDeviceCoordinator.h"
#include "FileChunkHandler.h"
#include "MemoryAllocation.h"
#include "Watchdog.h"

constexpr auto DEFAULT_CHUNK_SIZE = 1024;

class JobScheduler {

	/**
	 * \brief List of all device coordinators
	 */
	std::vector<std::shared_ptr<ClDeviceCoordinator>> clDeviceCoordinators;

	/**
	 * \brief Coordinator for CPU (SMP). This is shared_ptr to allow for polymorphism
	 */
	std::shared_ptr<CpuDeviceCoordinator> cpuDeviceCoordinator = nullptr;

	/**
	 * \brief To synchronize with device coordinators we use a semaphore which is incremented by coordinator after
	 * finishing a job
	 */
	ConcurrencyUtils::Semaphore jobFinishedSemaphore = ConcurrencyUtils::Semaphore(0);
	std::mutex coordinatorMutex;

	FileChunkHandler fileChunkHandler;

	Watchdog watchdog;

	size_t currentJobId = 0;

	std::vector<StatsAccumulator> statsAccumulators;

public:
	explicit JobScheduler(ProcessingInfo& processingInfo, const size_t chunkSize = DEFAULT_CHUNK_SIZE):
		fileChunkHandler(processingInfo.DistFilePath, chunkSize) {
		auto coordinatorId = 0;

		// Create memory configuration
		auto memoryConfig = MemoryAllocation::buildMemoryConfig(processingInfo);


		// Add CL devices
		for (const auto& [platform, devices] : processingInfo.Devices) {
			for (const auto& device : devices) {
				clDeviceCoordinators.push_back(std::make_shared<ClDeviceCoordinator>(
						CoordinatorType::OPEN_CL,
						processingInfo.ProcessingMode,
						// Call member function of this
						[this](auto&& ph1, auto&& ph2) {
							jobFinishedCallback(std::forward<decltype(ph1)>(ph1), std::forward<decltype(ph2)>(ph2));
						},
						chunkSize,
						memoryConfig.BytesPerClAccumulator,
						memoryConfig.MaxClHostBufferSizeBytes,
						processingInfo.DistFilePath,
						coordinatorId,
						platform,
						device
					)
				);
				coordinatorId += 1;
			}
		}

		// Add CPU device coordinator - this will be set to inactive state if OPENCL_DEVICES mode is used
		// If CPU supports AVX2 then use AVX2 capable coordinator
		cpuDeviceCoordinator = __ISA_AVAILABLE_AVX2
			                       ? std::make_shared<Avx2CpuDeviceCoordinator>(
				                       CoordinatorType::TBB,
				                       processingInfo.ProcessingMode,
				                       [this](auto&& ph1, auto&& ph2) {
					                       jobFinishedCallback(std::forward<decltype(ph1)>(ph1),
					                                           std::forward<decltype(ph2)>(ph2));
				                       },
				                       chunkSize,
				                       memoryConfig.BytesPerCpuAccumulator,
				                       memoryConfig.MaxCpuBufferSizeBytes,
				                       processingInfo.DistFilePath,
				                       coordinatorId,
				                       processingInfo.ProcessingMode == ProcessingMode::SINGLE_THREAD
					                       ? 1
					                       : std::thread::hardware_concurrency()
			                       )
			                       : std::make_shared<CpuDeviceCoordinator>(
				                       CoordinatorType::TBB,
				                       processingInfo.ProcessingMode,
				                       [this](auto&& ph1, auto&& ph2) {
					                       jobFinishedCallback(std::forward<decltype(ph1)>(ph1),
					                                           std::forward<decltype(ph2)>(ph2));
				                       },
				                       chunkSize,
				                       memoryConfig.BytesPerCpuAccumulator,
				                       memoryConfig.MaxCpuBufferSizeBytes,
				                       processingInfo.DistFilePath,
				                       coordinatorId,
				                       processingInfo.ProcessingMode == ProcessingMode::SINGLE_THREAD
					                       ? 1
					                       : std::thread::hardware_concurrency()
			                       );
	}

	/**
	 * \brief Returns whether there is any job remaining
	 * \return true if there is any job remaining, false otherwise
	 */
	[[nodiscard]] auto jobRemaining() const {
		// Job is available when there are still some chunks left to process
		return !fileChunkHandler.allChunksProcessed();
	}

	/**
	 * \brief Returns if there is any coordinator available
	 * \return true if there is any coordinator available, false otherwise
	 */
	bool anyCoordinatorAvailable() {
		auto scopedLock = std::scoped_lock(coordinatorMutex);
		return std::any_of(clDeviceCoordinators.begin(), clDeviceCoordinators.end(), [](const auto& coordinator) {
			return coordinator->available() && coordinator->enabled();
		}) || cpuDeviceCoordinator->available() && cpuDeviceCoordinator->enabled();
	}

	/**
	 * \brief Returns if there is any coordinator processing job
	 * \return true if all coordinators are available, false otherwise
	 */
	bool allCoordinatorsAvailable() {
		auto scopedLock = std::scoped_lock(coordinatorMutex);
		return std::all_of(clDeviceCoordinators.begin(), clDeviceCoordinators.end(), [](const auto& coordinator) {
			return coordinator->available();
		}) && cpuDeviceCoordinator->available();
	}

	/**
	 * \brief Gets next best available coordinator. Caller must ensure that there is at least one coordinator available
	 * \return shared pointer to given coordinator - this is cast to DeviceCoordinator
	 */
	auto getNextAvailableDeviceCoordinator() {
		// CL devices are ordered from best (0) to worst (n)
		// We prioritize CL devices over SMP
		const auto clDevice = std::find_if(clDeviceCoordinators.begin(), clDeviceCoordinators.end(),
		                                   [](const auto& coordinator) {
			                                   return coordinator->enabled() && coordinator->available();
		                                   });

		return clDevice != clDeviceCoordinators.end()
			       ? std::dynamic_pointer_cast<DeviceCoordinator>(*clDevice)
			       : std::dynamic_pointer_cast<DeviceCoordinator>(cpuDeviceCoordinator);
	}

	void addProcessedJob(const std::unique_ptr<Job> job, const size_t coordinatorIdx) {
		for (const auto& statsAcc : job->Result) {
			statsAccumulators.push_back(statsAcc);
		}
	}

	void jobFinishedCallback(std::unique_ptr<Job> job, const size_t coordinatorIdx) {
		auto scopedLock = std::scoped_lock(coordinatorMutex);

		// Write data to job aggregator
		addProcessedJob(std::move(job), coordinatorIdx);
		watchdog.updateCounter(1);
		jobFinishedSemaphore.release();
	}

	void terminateDeviceCoordinators() const {
		for (const auto& clCoordinator : clDeviceCoordinators) {
			clCoordinator->terminate();
		}

		cpuDeviceCoordinator->terminate();
	}

	void assignJob() {
		auto scopedLock = std::scoped_lock(coordinatorMutex);

		// Get the coordinator
		const auto coordinator = getNextAvailableDeviceCoordinator();

		// Build and assign new job for them
		const auto chunkRange = fileChunkHandler.getNextNChunks(coordinator->getMaxNumberOfChunks());
		coordinator->assignJob(Job(chunkRange, currentJobId));
		currentJobId += 1;
	}

	/**
	 * \brief Runs the job scheduler.
	 */
	inline auto run() {
		while (true) {
			// Check if there is a job available
			if (!jobRemaining()) {
				// If there is not check if all coordinators have finished (i.e. are available)
				if (allCoordinatorsAvailable()) {
					break; // All coordinators are done - break the loop
				}

				// Wait for coordinators to finish
				jobFinishedSemaphore.acquire();
				continue;
			}

			// Else there is job available
			// Select best device available (prioritize GPU over CPU)
			if (!anyCoordinatorAvailable()) {
				jobFinishedSemaphore.acquire();
			}

			assignJob();
		}

		// Terminate watchdog
		watchdog.terminate();

		// Terminate all coordinators
		terminateDeviceCoordinators();

		return statsAccumulators;
	}
};
