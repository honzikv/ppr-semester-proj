#include <oneapi/tbb.h>

#include "CpuDeviceCoordinator.h"


CpuDeviceCoordinator::CpuDeviceCoordinator(const CoordinatorType coordinatorType, const ProcessingMode processingMode,
	const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback, const size_t chunkSizeBytes,
	const size_t bytesPerAccumulator, const size_t cpuBufferSizeBytes, fs::path& distFilePath, const size_t id): DeviceCoordinator(
		                      coordinatorType, processingMode, jobFinishedCallback, 
		                      chunkSizeBytes, bytesPerAccumulator, distFilePath, id) {
	maxNumberOfChunksPerJob = cpuBufferSizeBytes / chunkSizeBytes;
	startCoordinatorThread();
}

void CpuDeviceCoordinator::onProcessJob() {
	std::cout << "OnProcessJob CPU" << std::endl;

}
