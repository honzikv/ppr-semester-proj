#include "ClDeviceCoordinator.h"
#include "ClSources.h"
#include "StatUtils.h"
#include "Logging.h"

auto ClDeviceCoordinator::compile(const std::string& source, const std::string& programName,
	const cl::Context& deviceContext) {
	const auto program = cl::Program(deviceContext, source);
	if (const auto result = program.build(DEFAULT_BUILD_FLAG); result != CL_BUILD_SUCCESS) {
		throw ClCompileErr(
			"Error during OpenCL Program compilation ( " + programName + " )\n. Error: " + std::to_string(result));
	}

	return program;
}

void ClDeviceCoordinator::setup() {
	maxWorkGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
	const auto maxDeviceBufferSize = static_cast<size_t>(static_cast<double>(device.getInfo<
			CL_DEVICE_MAX_MEM_ALLOC_SIZE>())
		* VIDEO_MEMORY_SCALE);

	// This is (ideally) divisible without remainder
	const auto chunksPerAccumulator = static_cast<size_t>(std::ceil(
		static_cast<double>(bytesPerAccumulator) / static_cast<double>(chunkSizeBytes)));

	// Compute how many accumulators can a single workgroup process
	const auto accumulatorsPerJob = maxDeviceBufferSize / bytesPerAccumulator > maxWorkGroupSize
		                                ? maxWorkGroupSize
		                                : maxDeviceBufferSize / bytesPerAccumulator;

	// Total chunks we manage to process is accumulatorsPerJob * chunksPerAccumulator
	maxNumberOfChunksPerJob = accumulatorsPerJob * chunksPerAccumulator;

	// OpenCL boilerplate
	context = cl::Context(device);
	commandQueue = cl::CommandQueue(context, device);
	program = compile(CL_PROGRAM, "program", context);
	deviceName = device.getInfo<CL_DEVICE_NAME>();
}

void ClDeviceCoordinator::onProcessJob() {
	log(INFO, "[OpenCL] Processing job with id " + std::to_string(currentJob->Id) + " on device \"" + deviceName + "\"");
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
			currentJob->Items = {};
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

	// currentJob->Items = {StatUtils::mergePairwise(results)};
	currentJob->Items = results;
}
