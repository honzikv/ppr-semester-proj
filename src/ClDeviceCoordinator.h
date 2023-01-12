#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include "DeviceCoordinator.h"

namespace fs = std::filesystem;

constexpr auto BUFFER_MAX_SIZE_SCALE = .9;
constexpr auto KERNEL_NAME = "computeStats";
// 80% of video memory is used for computation (or rather 90% of what OpenCL returns)

constexpr auto DEFAULT_BUILD_FLAG = "-cl-std=CL2.0";

constexpr auto N_CL_OUT_ITEMS = 7;
// Indices in the array
constexpr auto N_ITEMS_IDX = 0;
constexpr auto M1_IDX = 1;
constexpr auto M2_IDX = 2;
constexpr auto M3_IDX = 3;
constexpr auto M4_IDX = 4;
constexpr auto INTEGER_ONLY_IDX = 5;
constexpr auto MIN_IDX = 6;

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
		CoordinatorType coordinatorType,
		ProcessingMode processingMode,
		const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
		const std::function<void(size_t)>& notifyWatchdogCallback,
		const std::function<void(CoordinatorErr)>& errCallback,
		size_t chunkSizeBytes,
		size_t bytesPerAccumulator,
		size_t clHostBufferSizeBytes,
		fs::path& distFilePath,
		size_t id,
		cl::Device device);

private:
	cl::Device device; // The actual device
	cl::Context context; // Cl context
	cl::CommandQueue commandQueue;
	cl::Program program; // Compiled program
	size_t maxWorkGroupSize{}; // Max number of work items in a work group
	size_t maxHostChunks;
	size_t chunksPerAccumulator{};
	std::string deviceName;
	std::string deviceType = "GPU";

	/**
	 * \brief Compiles given source into program
	 * \param source string containing source code to be compiled
	 * \param programName name of the program
	 * \param deviceContext device context
	 * \return cl::Program instance or throws ClCompileErr if the program cannot be compiled
	 */
	auto compile(const std::string& source, const std::string& programName, const cl::Context& deviceContext) const;

	/**
	 * \brief Sets up the device, throwing ClCompileErr if something goes wrong
	 */
	void setup();

	void throwIfStatusUnsuccessful(const cl_int clStatus) const;

	/**
	 * \brief Estimates workgroup size
	 */
	void estimateWorkgroupSize();

protected:
	/**
	 * \brief Function override to perform computation on OpenCL device
	 */
	void onProcessJob() override;


};
