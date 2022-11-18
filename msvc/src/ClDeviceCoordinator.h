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
	                    const std::function<void(std::unique_ptr<Job>)>& jobFinishedCallback,
	                    size_t memoryLimit,
	                    size_t chunkSize,
	                    fs::path& distFilePath,
	                    const cl::Platform& platform,
	                    const cl::Device& device,
	                    const size_t id
	) : DeviceCoordinator(coordinatorType, processingMode, jobFinishedCallback, memoryLimit, chunkSize, distFilePath,
	                      id),
	    platform(platform),
	    device(device) {
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
