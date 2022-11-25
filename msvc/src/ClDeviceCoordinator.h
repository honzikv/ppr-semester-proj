#pragma once

#include <CL/cl.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>


#include "DeviceCoordinator.h"
#include "ClSources.h"

namespace fs = std::filesystem;

constexpr auto VIDEO_MEMORY_SCALE = .8;
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
	size_t workGroupSize{}; // Max number of work items in a work group
	size_t maxHostChunks;
	size_t bufferSizePerWorkerBytes{};

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
		workGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
		const auto bufferSize = static_cast<size_t>(static_cast<double>(device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>())
			* VIDEO_MEMORY_SCALE);

		// Calculate max number of chunks which is basically size of the buffer split into chunks that are split for threads
		bufferSizePerWorkerBytes = bufferSize / chunkSizeBytes / workGroupSize;
		maxNumberOfChunksPerJob = bufferSizePerWorkerBytes * workGroupSize;

		// OpenCL boilerplate
		context = cl::Context(device);
		commandQueue = cl::CommandQueue(context, device);
		program = compile(CL_PROGRAM, "program", context);
	}

protected:
	void onProcessJob() override {
		std::cout << "Processing job on CL device" << std::endl;
		auto dataLoader = DataLoader(filePath, chunkSizeBytes);
		const auto deviceBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, 512 * 1024 * 1024);
		const auto chunksLoaded = dataLoader.loadJobDataIntoDeviceBuffer(
			*currentJob, maxHostChunks, commandQueue, deviceBuffer);

		// Compute the number of workers
		auto nWorkers = chunksLoaded / bufferSizePerWorkerBytes;
		auto itemsPerWorker = bufferSizePerWorkerBytes / sizeof(double);
		if (nWorkers == 0) {
			// This means that the job is smaller than buffer size for a single worker
			// So the job will be done only on one worker
			itemsPerWorker = chunksLoaded * dataLoader.ChunkSizeBytes / sizeof(double);
			nWorkers = 1;
		}

		// Get the kernel
		auto kernel = cl::Kernel(program, KERNEL_NAME);

		// Pass the params
		kernel.setArg(0, deviceBuffer);
		// kernel.setArg(1, outputBuffer);
		kernel.setArg(1, itemsPerWorker);

		// Run the kernel
		commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, nWorkers);

		// Read back to host
		auto output = std::vector<double>(nWorkers * N_CL_OUT_PARAMS);
		commandQueue.enqueueReadBuffer(deviceBuffer, CL_TRUE, 0, output.size() * sizeof(double), output.data());

		// Aggregate the results
		auto results = std::vector<StatsAccumulator>(nWorkers);
		for (auto workerId = 0ULL; workerId < nWorkers; workerId += 1) {
			results[workerId] = {
				static_cast<size_t>(output[workerId * 6]),
				output[workerId * 6 + 1],
				output[workerId * 6 + 2],
				output[workerId * 6 + 3],
				output[workerId * 6 + 4],
				static_cast<bool>(output[workerId * 6 + 5]),
			};
		}

		for (auto i = 0; i < results.size() / 2; i += 2) {
			results[i] = results[i] + results[i + 1];
		}

		currentJob->Result = results;
	}
};
