#pragma once

#include "DeviceCoordinator.h"
#include <CL/cl.hpp>
#include <filesystem>


namespace fs = std::filesystem;

/**
 * \brief Wraps OpenCL logic for device coordination
 */
class ClDeviceCoordinator final : public DeviceCoordinator {
public:
	ClDeviceCoordinator(const CoordinatorType coordinatorType,
	                    const ProcessingMode processingMode,
	                    const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
	                    const size_t memoryLimit,
	                    const size_t chunkSize,
	                    fs::path& distFilePath,
	                    const cl::Platform& platform,
	                    const cl::Device& device,
	                    const size_t id
	) : DeviceCoordinator(coordinatorType, processingMode, jobFinishedCallback, memoryLimit, chunkSize, distFilePath,
	                      id),
	    platform(platform),
	    device(device) {

		// First we need to analyze the device
		// Find out how much memory we can allocate for our computation
		analyzeDevice();

		// Next setup openCL 
		setupOpenCl();

		// After we have set everything up start the thread
		startCoordinatorThread();

	}

protected:
	virtual void onProcessJob() override;

private:
	cl::Platform platform; // Device platform
	cl::Device device; // The actual device
	cl::Context context; // Cl context
	cl::CommandQueue commandQueue; // Command queue
	std::vector<cl::Program> kernels; // Contains all programs

	void analyzeDevice() {
		
	}

	void setupOpenCl() {
		
	}
};
