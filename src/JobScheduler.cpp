#include "JobScheduler.h"

JobScheduler::JobScheduler(ProcessingConfig& processingConfig, size_t chunkSizeBytes) {
	watchdog = std::make_unique<Watchdog>(std::chrono::milliseconds{processingConfig.WatchdogTimeoutMs});
	if (const auto fileSize = fs::file_size(processingConfig.DistFilePath);
		fileSize < chunkSizeBytes || fileSize < SMALL_SIZE_LIMIT) {
		chunkSizeBytes = 1; // Set chunk size to 1 - this way all bytes are processed
	}
	fileChunkHandler = std::make_unique<FileChunkHandler>(processingConfig.DistFilePath, chunkSizeBytes);

	// Create memory configuration
	const auto bytesPerCpuAccumulator = processingConfig.UseAvx2Instructions
		                                    ? DEFAULT_BYTES_PROCESSED_BY_ACCUMULATOR_CPU
		                                    : DEFAULT_BYTES_PROCESSED_BY_ACCUMULATOR_CPU * 4;
	auto memoryConfig = MemoryAllocation::buildMemoryConfig(processingConfig,
	                                                        bytesPerCpuAccumulator,
	                                                        DEFAULT_BYTES_PROCESSED_BY_ACCUMULATOR_CL,
	                                                        processingConfig.MemoryLimit);
	auto coordinatorId = 0;
	// Add CL devices
	for (const auto& device : processingConfig.ClDevices) {
		try {
			clDeviceCoordinators.push_back(std::make_shared<ClDeviceCoordinator>(
				CoordinatorType::OPEN_CL,
				processingConfig.ProcessingMode,
				// Use dark magic to pass member function as a callback
				[this](auto&& ph1, auto&& ph2) {
					jobFinishedCallback(std::forward<decltype(ph1)>(ph1), std::forward<decltype(ph2)>(ph2));
				},
				[this](auto&& ph1) {
					notifyWatchdogCallback(std::forward<decltype(ph1)>(ph1));
				},
					[this](auto&& ph1) {
					notifyErrOccurred(std::forward<decltype(ph1)>(ph1));
				},
					chunkSizeBytes,
					memoryConfig.BytesPerClAccumulator,
					memoryConfig.MaxClHostBufferSizeBytes,
					processingConfig.DistFilePath,
					coordinatorId,
					device
					)
			);
		}
		catch (ClCompileErr& err) {
			const auto deviceName = device.getInfo<CL_DEVICE_NAME>();
			log(CRITICAL, "Failed to compile OpenCL program for device. The program cannot continue. Device: " + deviceName + ". Error: " + err.what());
			exit(1);
		}
		coordinatorId += 1;
	}

	// Add CPU device coordinator - this will be set to inactive state if OPENCL_DEVICES mode is used
	// If CPU supports AVX2 then use AVX2 capable coordinator
	// ReSharper disable once CppRedundantBooleanExpressionArgument
	cpuDeviceCoordinator = static_cast<bool>(__ISA_AVAILABLE_AVX2) && processingConfig.UseAvx2Instructions
		                       ? std::make_shared<Avx2CpuDeviceCoordinator>(
			                       CoordinatorType::TBB,
			                       processingConfig.ProcessingMode,
			                       // Use dark magic to pass member function as a callback
			                       [this](auto&& ph1, auto&& ph2) {
				                       jobFinishedCallback(std::forward<decltype(ph1)>(ph1),
				                                           std::forward<decltype(ph2)>(ph2));
			                       },
			                       [this](auto&& ph1) {
				                       notifyWatchdogCallback(std::forward<decltype(ph1)>(ph1));
			                       },
			                       [this](auto&& ph1) {
				                       notifyErrOccurred(std::forward<decltype(ph1)>(ph1));
			                       },
			                       chunkSizeBytes,
			                       memoryConfig.BytesPerCpuAccumulator,
			                       memoryConfig.MaxCpuBufferSizeBytes,
			                       processingConfig.DistFilePath,
			                       coordinatorId
		                       )
		                       : std::make_shared<CpuDeviceCoordinator>(
			                       CoordinatorType::TBB,
			                       processingConfig.ProcessingMode,
			                       // Use dark magic to pass member function as a callback
			                       [this](auto&& ph1, auto&& ph2) {
				                       jobFinishedCallback(std::forward<decltype(ph1)>(ph1),
				                                           std::forward<decltype(ph2)>(ph2));
			                       },
			                       [this](auto&& ph1) {
				                       notifyWatchdogCallback(std::forward<decltype(ph1)>(ph1));
			                       },
			                       [this](auto&& ph1) {
				                       notifyErrOccurred(std::forward<decltype(ph1)>(ph1));
			                       },
			                       chunkSizeBytes,
			                       memoryConfig.BytesPerCpuAccumulator,
			                       memoryConfig.MaxCpuBufferSizeBytes,
			                       processingConfig.DistFilePath,
			                       coordinatorId);

	// Allocate coordinator availability array
	if (processingConfig.ProcessingMode == ProcessingMode::OPENCL_DEVICES) {
		coordinatorAvailability.resize(clDeviceCoordinators.size(), true);
	}
	else {
		coordinatorAvailability.resize(clDeviceCoordinators.size() + 1, true);
	}
}

JobScheduler::~JobScheduler() {
	// Once JobScheduler is destroyed join all threads allocated by it
	if (watchdog) {
		watchdog->join();
	}

	for (const auto& coordinator : clDeviceCoordinators) {
		if (coordinator) {
			coordinator->join();
		}
	}

	cpuDeviceCoordinator->join();
}

bool JobScheduler::anyCoordinatorAvailable() {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	return std::any_of(coordinatorAvailability.begin(), coordinatorAvailability.end(),
	                   [](const auto& available) { return available; });
}

bool JobScheduler::allCoordinatorsAvailable() {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	return std::all_of(coordinatorAvailability.begin(), coordinatorAvailability.end(), [](const auto& available) {
		return available;
	});
}

std::pair<size_t, std::shared_ptr<DeviceCoordinator>> JobScheduler::getNextAvailableDeviceCoordinator() {
	// We prioritize CL devices over SMP
	for (auto coordinatorId = 0ULL; coordinatorId < coordinatorAvailability.size(); coordinatorId += 1) {
		if (!coordinatorAvailability[coordinatorId]) {
			continue;
		}

		if (coordinatorId < clDeviceCoordinators.size()) {
			return {coordinatorId, std::dynamic_pointer_cast<DeviceCoordinator>(clDeviceCoordinators[coordinatorId])};
		}

		return {coordinatorId, std::dynamic_pointer_cast<DeviceCoordinator>(cpuDeviceCoordinator)};
	}

	// No coordinator available
	return {0, nullptr};

}

void JobScheduler::addProcessedJob(const std::unique_ptr<Job> job) {
	for (const auto& accumulator : job->Items) {
		accumulators.push_back(accumulator);
	}

	log(INFO, "[JOBSCHEDULER] Job " + std::to_string(job->Id) + " was successfully processed");
}

void JobScheduler::jobFinishedCallback(std::unique_ptr<Job> job, const size_t coordinatorIdx) {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	coordinatorAvailability[coordinatorIdx] = true;
	addProcessedJob(std::move(job));
	jobFinishedSemaphore.release();
}

void JobScheduler::notifyWatchdogCallback(const size_t bytesProcessed) {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	watchdog->updateCounter(bytesProcessed);
}

void JobScheduler::notifyErrOccurred(const CoordinatorErr& err) {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	jobFinishedSemaphore.release();
	if (!lastErr) {
		// If there is no last erorr we can set this
		lastErr = std::make_unique<CoordinatorErr>(err);
		return;
	}

	// Otherwise we need to check whether the last error was fatal - if not we can log it
	if (lastErr->IsFatal) {
		return;
	}

	log(WARNING,
	    "[JOBSCHEDULER] Coordinator " + std::to_string(lastErr->CoordinatorId) + " encountered an error: " +
	    lastErr->What);
	lastErr = std::make_unique<CoordinatorErr>(err);
}

void JobScheduler::terminateDeviceCoordinators() const {
	for (const auto& clCoordinator : clDeviceCoordinators) {
		if (clCoordinator) {
			clCoordinator->terminate();
		}
	}

	if (cpuDeviceCoordinator) {
		cpuDeviceCoordinator->terminate();
	}
}

void JobScheduler::assignJob() {
	checkForErrors();
	auto scopedLock = std::scoped_lock(coordinatorMutex);

	// Get the coordinator
	const auto [coordinatorId, coordinator] = getNextAvailableDeviceCoordinator();
	coordinatorAvailability[coordinatorId] = false;

	// Build and assign new job for them
	const auto chunkRange = fileChunkHandler->getNextNChunks(coordinator->getMaxNumberOfChunks());
	coordinator->assignJob(Job(chunkRange, currentJobId));
	currentJobId += 1;
}

void JobScheduler::checkForErrors() {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	if (!lastErr) {
		return;
	}

	if (lastErr->IsFatal) {
		log(CRITICAL,
		    "[JOBSCHEDULER] Coordinator " + std::to_string(lastErr->CoordinatorId) + " encountered an error: " +
		    lastErr->What);
		terminateDeviceCoordinators();
		throw std::runtime_error("Error occurred during computation, the program cannot continue");
	}

	// Otherwise log it as a warning=
	log(WARNING,
	    "[JOBSCHEDULER] Coordinator " + std::to_string(lastErr->CoordinatorId) + " encountered an error: " +
	    lastErr->What);
}

std::vector<StatsAccumulator> JobScheduler::run() {
	log(DEBUG, "[JOBSCHEDULER] Starting Job Scheduler");
	// Start the watchdog - by this time all device coordinators are waiting for jobs
	watchdog->start();
	while (true) {
		checkForErrors();

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
			continue;
		}

		assignJob();
	}
	log(DEBUG, "[JOBSCHEDULER] Job finished, terminating device coordinators and watchdog.");
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	// Terminate watchdog
	watchdog->terminate();

	// Terminate all coordinators
	terminateDeviceCoordinators();

	return accumulators;
}
