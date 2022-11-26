#pragma once

#include <CL/cl.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "DeviceCoordinator.h"
#include "ClSources.h"
#include "StatUtils.h"
#define CL_HPP_ENABLE_EXCEPTIONS

namespace fs = std::filesystem;

constexpr auto VIDEO_MEMORY_SCALE = .5;
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
		const cl::Platform& platform,
		const cl::Device& device)
		: DeviceCoordinator(
			  coordinatorType, processingMode, jobFinishedCallback, chunkSizeBytes,
			  bytesPerAccumulator, distFilePath, id),
		  platform(platform),
		  device(device),
		  maxHostChunks(clHostBufferSizeBytes / chunkSizeBytes) {
		// Setup the device
		setup();

		// After we have set everything up start the thread
		startCoordinatorThread();
	}

private:
	cl::Platform platform; // Device platform
	cl::Device device; // The actual device
	cl::Context context; // Cl context
	cl::CommandQueue commandQueue; // Command queue
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
	static auto compile(const std::string& source, const std::string& programName, const cl::Context& deviceContext) {
		const auto program = cl::Program(deviceContext, source);
		if (const auto result = program.build(DEFAULT_BUILD_FLAG); result != CL_BUILD_SUCCESS) {
			throw ClCompileErr(
				"Error during OpenCL Program compilation ( " + programName + " )\n. Error: " + std::to_string(result));
		}

		return program;
	}

	void setup() {
		maxWorkGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
		auto mem = static_cast<double>(device.getInfo<
			CL_DEVICE_MAX_MEM_ALLOC_SIZE>());
		const auto maxDeviceBufferSize = static_cast<size_t>(static_cast<double>(device.getInfo<
				CL_DEVICE_MAX_MEM_ALLOC_SIZE>())
			* VIDEO_MEMORY_SCALE);

		// Per job we schedule one work group - therefore we need to make sure that there is enough memory to fit in all the data
		maxNumberOfChunksPerJob = bytesPerAccumulator * maxWorkGroupSize > maxDeviceBufferSize
			? maxDeviceBufferSize / bytesPerAccumulator / chunkSizeBytes
			                         : bytesPerAccumulator * maxWorkGroupSize / chunkSizeBytes;

		// OpenCL boilerplate
		context = cl::Context(device);
		commandQueue = cl::CommandQueue(context, device);
		program = compile(CL_PROGRAM, "program", context);
		deviceName = device.getInfo<CL_DEVICE_NAME>();
	}

protected:
	void onProcessJob() override {
		log(INFO, "Processing job with id " + std::to_string(currentJob->Id) + " on OpenCL device: " + deviceName);
		// Create data loader
		auto dataLoader = DataLoader(filePath, chunkSizeBytes);

		const auto [deviceBuffer, nChunksLoaded] = dataLoader.loadJobDataIntoDeviceBuffer(
			*currentJob, maxHostChunks, commandQueue, context);

		// Compute number of work items
		auto nWorkItems = nChunksLoaded * chunkSizeBytes / bytesPerAccumulator / sizeof(double);
		auto itemsPerWorker = bytesPerAccumulator / sizeof(double);

		if (nWorkItems == 0) {
			if (nChunksLoaded == 0) {
				// If no chunks were loaded then there is nothing to process, return
				currentJob->Result = {};
				return;
			}
			// The work is too small so we will just compute it in one work item
			nWorkItems = 1;
			itemsPerWorker = nChunksLoaded * chunkSizeBytes / sizeof(double);
		}

		// Get the kernel
		auto kernel = cl::Kernel(program, KERNEL_NAME);

		// Pass the params
		kernel.setArg(0, deviceBuffer);
		// kernel.setArg(1, outputBuffer);
		kernel.setArg(1, static_cast<size_t>(itemsPerWorker));

		// Run the kernel
		int res = commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, nWorkItems);

		// Read back to host
		auto output = std::vector<double>(nWorkItems * N_CL_OUT_PARAMS);
		commandQueue.enqueueReadBuffer(deviceBuffer, CL_TRUE, 0, output.size() * sizeof(double), output.data());

		// Read out the results
		auto results = std::vector<StatsAccumulator>(nWorkItems);
		for (auto workerId = 0ULL; workerId < nWorkItems; workerId += 1) {
			results[workerId] = {
				static_cast<size_t>(output[workerId * 6]),
				output[workerId * 6 + 1],
				output[workerId * 6 + 2],
				output[workerId * 6 + 3],
				output[workerId * 6 + 4],
				static_cast<bool>(output[workerId * 6 + 5]),
			};
		}

		currentJob->Result = {StatUtils::mergePairwise(results)};
		// currentJob->Result = results;
	}
};
