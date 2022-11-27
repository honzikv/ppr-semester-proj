#pragma once
#include <CL/cl.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "ProcessingConfig.h"
#include "DeviceCoordinator.h"


#define CL_HPP_ENABLE_EXCEPTIONS

namespace fs = std::filesystem;

constexpr auto VIDEO_MEMORY_SCALE = .75;
constexpr auto KERNEL_NAME = "computeStats";
// 80% of video memory is used for computation (or rather 90% of what OpenCL returns)

constexpr auto DEFAULT_BUILD_FLAG = "-cl-std=CL2.0";

constexpr auto N_CL_OUT_PARAMS = 6;

/**
 * \brief Custom error for control flow
 */
class ClCompileErr final : public std::runtime_error {
public:
	explicit ClCompileErr(const std::string& what = "") : std::runtime_error(what) {
	}
};


/**
 * \brief Wraps OpenCL logic for device coordination
 */
class ClDeviceCoordinator final : public DeviceCoordinator {


public:
	ClDeviceCoordinator(
		const CoordinatorType coordinatorType,
		const ProcessingMode processingMode,
		const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
		const size_t chunkSizeBytes,
		const size_t bytesPerAccumulator,
		const size_t clHostBufferSizeBytes,
		fs::path& distFilePath,
		const size_t id,
		const cl::Device& device)
		: DeviceCoordinator(
			  coordinatorType, processingMode, jobFinishedCallback, chunkSizeBytes,
			  bytesPerAccumulator, distFilePath, id),
		  device(device),
		  maxHostChunks(clHostBufferSizeBytes / chunkSizeBytes) {
		// Setup the device
		setup();

		// After we have set everything up start the thread
		startCoordinatorThread();
	}

private:
	cl::Device device; // The actual device
	cl::Context context; // Cl context
	cl::CommandQueue commandQueue;
	cl::Program program; // Compiled program
	size_t maxWorkGroupSize{}; // Max number of work items in a work group
	size_t maxHostChunks;
	std::string deviceName;

	/**
	 * \brief Compiles given source into program
	 * \param source string containing source code to be compiled
	 * \param programName name of the program
	 * \param deviceContext device context
	 * \param device device to compile for
	 * \return cl::Program instance or throws ClCompileErr if the program cannot be compiled
	 */
	static auto compile(const std::string& source, const std::string& programName, const cl::Context& deviceContext);

	/**
	 * \brief Sets up the device, throwing ClCompileErr if something goes wrong
	 */
	void setup();

protected:

	/**
	 * \brief Function override to perform computation on OpenCL device
	 */
	void onProcessJob() override;
};
