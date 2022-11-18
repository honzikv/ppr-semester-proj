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
	std::shared_ptr<CpuDeviceCoordinator> cpuDeviceCoordinator = nullptr;

	Utils::Semaphore jobFinishedSemaphore = Utils::Semaphore();

	FileChunkHandler fileChunkHandler;
	std::unique_ptr<JobAggregator> jobAggregator = nullptr;

public:
	explicit JobScheduler(ProcessingInfo& processingInfo, const size_t chunkSize = DEFAULT_CHUNK_SIZE):
		fileChunkHandler(processingInfo.DistFilePath, chunkSize) {
		auto coordinatorId = 0;

		// Compute memory limits
		const auto memoryLimits = MemoryUtils::computeCoordinatorMemoryLimits(processingInfo);
		const auto clDeviceMemory = memoryLimits.ClDeviceMemory / memoryLimits.ClDeviceCount;

		// Add CL device if OpenCL mode
		for (const auto& [platform, devices] : processingInfo.Devices) {
			for (const auto& device : devices) {
				clDeviceCoordinators.push_back(std::make_shared<ClDeviceCoordinator>(
						CoordinatorType::OPEN_CL,
						processingInfo.ProcessingMode,
						[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
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

		cpuDeviceCoordinator = std::make_shared<CpuDeviceCoordinator>(
			CoordinatorType::TBB,
			processingInfo.ProcessingMode,
			[this](auto&& ph1) { jobFinishedCallback(std::forward<decltype(ph1)>(ph1)); },
			memoryLimits.CpuMemory,
			DEFAULT_CHUNK_SIZE,
			processingInfo.DistFilePath,
			coordinatorId
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
	auto anyCoordinatorAvailable() {
		return std::any_of(clDeviceCoordinators.begin(), clDeviceCoordinators.end(), [](const auto& coordinator) {
			return coordinator->available() && coordinator->active();
		}) || cpuDeviceCoordinator->available() && cpuDeviceCoordinator->active();
	}

	/**
	 * \brief Returns if there is any coordinator processing job
	 * \return true if all coordinators are available, false otherwise
	 */
	auto allCoordinatorsAvailable() {
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

	void jobFinishedCallback(std::unique_ptr<Job> job) {

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

			// Get the coordinator
			const auto coordinator = getNextAvailableDeviceCoordinator();

			// Build and assign new job for them
			const auto chunkRange = fileChunkHandler.getNextNChunks(coordinator->getMaxNumberOfChunks());
			coordinator->assignJob(Job(chunkRange));
		}
	}
};
