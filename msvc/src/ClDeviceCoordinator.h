#pragma once

#include <CL/cl.hpp>
#include <filesystem>
#include "DeviceCoordinator.h"
#include "ClSources.h"

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
		// Setup openCL device
		setup();

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
	cl::Program program; // Compiled program

	void setup() {
		const auto bufferSize = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
		const auto workGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();

		maxNumberOfChunks = (floor(bufferSize / workGroupSize) * workGroupSize) / chunkSize;
		context = cl::Context(device);
		commandQueue = cl::CommandQueue(context, device);

		// Compile the program
		program = compile(program, "program", context);
	}
};
