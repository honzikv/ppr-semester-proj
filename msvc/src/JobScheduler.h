#pragma once
#include "CpuDeviceCoordinator.h"
#include "Arghandling.h"
#include "ClDeviceCoordinator.h"
#include "FileChunkHandler.h"
#include "JobAggregator.h"
#include "MemoryUtils.h"

constexpr auto DEFAULT_CHUNK_SIZE = 4096;

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
	ConcurrencyUtils::Semaphore jobFinishedSemaphore = ConcurrencyUtils::Semaphore(1);

	FileChunkHandler fileChunkHandler;

	/**
	 * \brief Job aggregator instance which is used to store the result. This is unique_ptr for it
	 * to initialized later in the constructor
	 */
	std::unique_ptr<JobAggregator> jobAggregator = nullptr;

public:
	explicit JobScheduler(ProcessingInfo& processingInfo, const size_t chunkSize = DEFAULT_CHUNK_SIZE):
		fileChunkHandler(processingInfo.DistFilePath, chunkSize) {
		auto coordinatorId = 0;

		// Compute memory limits
		const auto memoryLimits = MemoryUtils::computeCoordinatorMemoryLimits(processingInfo);
		const auto clDeviceMemory = memoryLimits.ClDeviceCount == 0
			                            ? 0
			                            : memoryLimits.ClDeviceMemory / memoryLimits.ClDeviceCount;

		// Add CL devices if any
		for (const auto& [platform, devices] : processingInfo.Devices) {
			for (const auto& device : devices) {
				clDeviceCoordinators.push_back(std::make_shared<ClDeviceCoordinator>(
						CoordinatorType::OPEN_CL,
						processingInfo.ProcessingMode,
						[this](auto&& ph1, auto&& ph2) {
							jobFinishedCallback(std::forward<decltype(ph1)>(ph1), std::forward<decltype(ph2)>(ph2));
						},
						clDeviceMemory,
						DEFAULT_CHUNK_SIZE,
						processingInfo.DistFilePath,
						platform,
						device,
						coordinatorId
					)
				);
				coordinatorId += 1;
			}
		}

		// Add CPU device coordinator - this will be set to inactive state if OPENCL_DEVICES mode is used
		cpuDeviceCoordinator = std::make_shared<CpuDeviceCoordinator>(
			CoordinatorType::TBB,
			processingInfo.ProcessingMode,
			[this](auto&& ph1, auto&& ph2) {
				jobFinishedCallback(std::forward<decltype(ph1)>(ph1), std::forward<decltype(ph2)>(ph2));
			},
			memoryLimits.CpuMemory,
			DEFAULT_CHUNK_SIZE,
			processingInfo.DistFilePath,
			coordinatorId
		);

		jobAggregator = std::make_unique<JobAggregator>(clDeviceCoordinators.size() +
		                                                processingInfo.ProcessingMode != ProcessingMode::OPENCL_DEVICES
			                                                ? 1
			                                                : 0);
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
		return std::any_of(clDeviceCoordinators.begin(), clDeviceCoordinators.end(), [](const auto& coordinator) {
			return coordinator->available() && coordinator->active();
		}) || cpuDeviceCoordinator->available() && cpuDeviceCoordinator->active();
	}

	/**
	 * \brief Returns if there is any coordinator processing job
	 * \return true if all coordinators are available, false otherwise
	 */
	bool allCoordinatorsAvailable() {
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
			                                   return coordinator->active() && coordinator->available();
		                                   });

		return clDevice != clDeviceCoordinators.end()
			       ? std::dynamic_pointer_cast<DeviceCoordinator>(*clDevice)
			       : std::dynamic_pointer_cast<DeviceCoordinator>(cpuDeviceCoordinator);
	}

	void jobFinishedCallback(std::unique_ptr<Job> job, const size_t coordinatorIdx) {
		// Write data to job aggregator
		jobAggregator->writeProcessedJob(std::move(job), coordinatorIdx);
		jobFinishedSemaphore.release();
	}

	void terminateDeviceCoordinators() const {
		for (const auto& clCoordinator : clDeviceCoordinators) {
			clCoordinator->terminate();
		}

		cpuDeviceCoordinator->terminate();
	}

	/**
	 * \brief Runs the job scheduler.
	 */
	inline auto run() {
		while (true) {
			// Check if there is job available
			if (!jobRemaining()) {
				// If not check if there are any coordinators working
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

			// Update processed jobs if there are any
			jobAggregator->update();

			// Get the coordinator
			const auto coordinator = getNextAvailableDeviceCoordinator();

			// Build and assign new job for them
			std::cout << "Assigning job!" << std::endl;
			const auto chunkRange = fileChunkHandler.getNextNChunks(coordinator->getMaxNumberOfChunks());
			coordinator->assignJob(Job(chunkRange));
		}

		// Update job aggregator
		jobAggregator->update();

		// Terminate all coordinators
		terminateDeviceCoordinators();

		return jobAggregator->getResult();
	}
};
