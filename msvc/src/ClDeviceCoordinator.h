#pragma once

#include "DeviceCoordinator.h"
#include <CL/cl.hpp>
#include <filesystem>


namespace fs = std::filesystem;

/**
 * \brief Wraps OpenCL logic for device coordination
 */
class ClDeviceCoordinator : DeviceCoordinator {
public:
	ClDeviceCoordinator(CoordinatorType coordinatorType, const std::function<void(Job)>& jobFinishedCallback,
	                    size_t memoryLimit, size_t chunkSize, fs::path& distFilePath, const cl::Platform& platform,
	                    const cl::Device& device)
		: DeviceCoordinator(coordinatorType, jobFinishedCallback, memoryLimit, chunkSize, distFilePath),
		  platform(platform),
		  device(device),
		  commandQueue(context, device) {
	}

protected:
	virtual void onProcessJob() override {

	}

private:
	cl::Platform platform; // Device platform
	cl::Device device; // The actual device
	cl::Context context; // Cl context
	cl::CommandQueue commandQueue;
	std::vector<cl::Program> kernels; // Contains all programs

};
