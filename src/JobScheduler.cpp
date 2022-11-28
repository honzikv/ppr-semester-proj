#include "JobScheduler.h"

JobScheduler::JobScheduler(ProcessingConfig& processingConfig, size_t chunkSizeBytes):
	watchdog(std::chrono::milliseconds{processingConfig.WatchdogTimeoutMs}) {
	if (const auto fileSize = fs::file_size(processingConfig.DistFilePath);
		fileSize < chunkSizeBytes || fileSize < SMALL_SIZE_LIMIT) {
		chunkSizeBytes = 1; // Set chunk size to 1 - this way all bytes are processed
	}
	fileChunkHandler = std::make_unique<FileChunkHandler>(processingConfig.DistFilePath, chunkSizeBytes);

	// Create memory configuration
	auto memoryConfig = MemoryAllocation::buildMemoryConfig(processingConfig, processingConfig.MemoryLimit);
	auto coordinatorId = 0;
	// Add CL devices
	for (const auto& device : processingConfig.ClDevices) {
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

}

bool JobScheduler::anyCoordinatorAvailable() {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	return std::any_of(clDeviceCoordinators.begin(), clDeviceCoordinators.end(), [](const auto& coordinator) {
		return coordinator->available() && coordinator->enabled();
	}) || cpuDeviceCoordinator->available() && cpuDeviceCoordinator->enabled();
}

bool JobScheduler::allCoordinatorsAvailable() {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	return std::all_of(clDeviceCoordinators.begin(), clDeviceCoordinators.end(), [](const auto& coordinator) {
		return coordinator->available();
	}) && cpuDeviceCoordinator->available();
}

std::shared_ptr<DeviceCoordinator> JobScheduler::getNextAvailableDeviceCoordinator() {
	// We prioritize CL devices over SMP
	const auto clDevice = std::find_if(clDeviceCoordinators.begin(), clDeviceCoordinators.end(),
	                                   [](const auto& coordinator) {
		                                   return coordinator->enabled() && coordinator->available();
	                                   });
	return clDevice != clDeviceCoordinators.end()
		       ? std::dynamic_pointer_cast<DeviceCoordinator>(*clDevice)
		       : std::dynamic_pointer_cast<DeviceCoordinator>(cpuDeviceCoordinator);
}

void JobScheduler::addProcessedJob(const std::unique_ptr<Job> job) {
	for (const auto& accumulator : job->Items) {
		accumulators.push_back(accumulator);
	}

	log(INFO, "[JOBSCHEDULER] Job " + std::to_string(job->Id) + " was successfully processed");
}

void JobScheduler::jobFinishedCallback(std::unique_ptr<Job> job, const size_t coordinatorIdx) {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	addProcessedJob(std::move(job));
	jobFinishedSemaphore.release();
}

void JobScheduler::notifyWatchdogCallback(const size_t bytesProcessed) {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
	watchdog.updateCounter(bytesProcessed);
}

void JobScheduler::notifyErrOccurred(const CoordinatorErr& err) {
	auto scopedLock = std::scoped_lock(coordinatorMutex);
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
	auto scopedLock = std::scoped_lock(coordinatorMutex);

	// Get the coordinator
	const auto coordinator = getNextAvailableDeviceCoordinator();

	// Build and assign new job for them
	const auto chunkRange = fileChunkHandler->getNextNChunks(coordinator->getMaxNumberOfChunks());
	coordinator->assignJob(Job(chunkRange, currentJobId));
	currentJobId += 1;
}

void JobScheduler::checkForErrors() const {
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
	// Start the watchdog - by this time all device coordinators are waiting for jobs
	watchdog.start();
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
		}

		assignJob();
	}
	{
		auto scopedLock = std::scoped_lock(coordinatorMutex);
		// Terminate watchdog
		watchdog.terminate();

		// Terminate all coordinators
		terminateDeviceCoordinators();
	}

	

	return accumulators;
}
