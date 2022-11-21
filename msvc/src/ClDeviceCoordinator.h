#pragma once

#include <CL/cl.hpp>
#include <filesystem>

#include "DeviceCoordinator.h"
#include "ClSources.h"
#include "ClCompiler.h"

namespace fs = std::filesystem;

constexpr auto VIDEO_MEMORY_SCALE = .9; // 90% of video memory is used for computation (or rather 90% of what OpenCL returns)

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
	size_t workgroupSize; // Max number of work items in a work group

	void setup() {
		this->workgroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
		const auto bufferSize = static_cast<double>(device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>()) * VIDEO_MEMORY_SCALE;
		const auto workGroupSize = static_cast<double>(this->workgroupSize);

		maxNumberOfChunks = static_cast<size_t>(floor(bufferSize / workGroupSize) * workGroupSize) / chunkSize;
		context = cl::Context(device);
		commandQueue = cl::CommandQueue(context, device);

		// Compile the program
		program = compile(CL_PROGRAM, "program", context);
	}
};
