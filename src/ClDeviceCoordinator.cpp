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
	// OpenCL boilerplate
	context = cl::Context(device);
	commandQueue = cl::CommandQueue(context, device);
	program = compile(CL_PROGRAM, "program", context);
	deviceName = device.getInfo<CL_DEVICE_NAME>();

	// Load the kernel
	const auto kernel = cl::Kernel(program, KERNEL_NAME);

	// Use CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE as a hint for the number of work items in a work group
	//https://www.intel.com/content/www/us/en/develop/documentation/iocl-opg/top/coding-for-the-intel-cpu-opencl-device/work-group-size-considerations.html
	maxWorkGroupSize = kernel.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(device);
	const auto maxDeviceBufferSize = static_cast<size_t>(static_cast<double>(device.getInfo<
			CL_DEVICE_MAX_MEM_ALLOC_SIZE>())
		* BUFFER_MAX_SIZE_SCALE);

	// This is (ideally) divisible without remainder
	chunksPerAccumulator = static_cast<size_t>(std::floor(
		static_cast<double>(bytesPerAccumulator) / static_cast<double>(chunkSizeBytes)));

	// Total chunks we manage to process is accumulatorsPerJob * chunksPerAccumulator
	maxNumberOfChunksPerJob = maxWorkGroupSize * chunksPerAccumulator;

	// If we get more host memory than device memory align host memory to device memory
	maxHostChunks = maxHostChunks * chunkSizeBytes > maxDeviceBufferSize
		? static_cast<size_t>(std::floor(static_cast<double>(maxDeviceBufferSize) / static_cast<double>(chunkSizeBytes)))
		: maxHostChunks;
}

void ClDeviceCoordinator::onProcessJob() {
	log(INFO, "[OpenCL] Processing job with id " + std::to_string(currentJob->Id) + " on device \"" + deviceName +
	    "\"");

	// To utilize the GPU fully we need to ideally use maximum workgroup size
	// Unfortunately, there is a tradeoff since we are limited by total application memory (i.e. 1 GB)
	// and OpenCL is not at all generous with memory allocation

	// We cannot allocate more than maxDeviceBufferSize at once since OpenCL does not copy it to the device without
	// using additional memory
	// Therefore, we split the job into smaller pieces where we at once load only maxDeviceBufferSize bytes or less
	// and let the device compute it

	// Get total number of accumulators
	const auto nAccumulators = currentJob->getNChunks() / chunksPerAccumulator;

	// Get total number of workgroup runs
	const auto totalBytes = nAccumulators * bytesPerAccumulator;
	const auto nWorkgroupRuns = static_cast<size_t>(std::ceil(static_cast<double>(totalBytes) /
		static_cast<double>(maxHostChunks * chunkSizeBytes))); // NOLINT(clang-diagnostic-implicit-int-float-conversion)

	// Create buffer for results
	auto clStatus = cl_int{};
	const auto accumulatorsBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, nAccumulators * 6 * sizeof(double), nullptr,
	                                           &clStatus);
	throwIfStatusUnsuccessful(clStatus);

	// Write true values for isInteger only
	auto accumulatorData = std::vector<double>(nAccumulators * 6, 0.0);
	for (auto i = 0ULL; i < nAccumulators; ++i) {
		accumulatorData[i * N_CL_OUT_ITEMS + INTEGER_ONLY_IDX] = static_cast<double>(true);
	}

	clStatus = commandQueue.enqueueWriteBuffer(accumulatorsBuffer, CL_TRUE, 0, accumulatorData.size() * sizeof(double),
	                                           accumulatorData.data());
	throwIfStatusUnsuccessful(clStatus);

	// Create buffer for data
	auto dataBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, maxHostChunks * chunkSizeBytes, nullptr, &clStatus);
	throwIfStatusUnsuccessful(clStatus);

	// Load the kernel
	auto kernel = cl::Kernel(program, KERNEL_NAME);

	// Run the computations
	auto bytesRemaining = totalBytes;
	const auto [startIdx, endIdx] = currentJob->ChunkIdxRange;
	auto currentIdx = startIdx;
	int i = 0;
	while (currentIdx < endIdx && bytesRemaining > 0) {
		// Now keep loading data into the buffer
		const auto chunksToLoad = bytesRemaining > maxHostChunks * chunkSizeBytes
			                          ? maxHostChunks
			                          : bytesRemaining / chunkSizeBytes;
		dataLoader.loadChunksIntoDeviceBuffer(currentIdx, currentIdx + chunksToLoad, dataBuffer, commandQueue);

		const auto itemsToProcess = (chunksToLoad * chunkSizeBytes) / nAccumulators / sizeof(double);

		kernel.setArg(0, dataBuffer);
		kernel.setArg(1, accumulatorsBuffer);
		kernel.setArg(2, itemsToProcess);

		// Run the kernel
		clStatus = commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(nAccumulators),
		                                             cl::NDRange(nAccumulators));
		throwIfStatusUnsuccessful(clStatus);

		currentIdx = currentIdx + chunksToLoad;
		bytesRemaining -= chunksToLoad * chunkSizeBytes;
		i += 1;
		std::cout << "Run" << i << std::endl;
	}

	// Read out the results
	clStatus = commandQueue.enqueueReadBuffer(accumulatorsBuffer, CL_TRUE, 0, accumulatorData.size() * sizeof(double),
	                                          accumulatorData.data());
	throwIfStatusUnsuccessful(clStatus);

	// Map the results
	auto results = std::vector<StatsAccumulator>(nAccumulators);
	for (auto workerId = 0ULL; workerId < nAccumulators; workerId += 1) {
		const auto offset = workerId * N_CL_OUT_ITEMS;
		results[workerId] = {
			static_cast<size_t>(accumulatorData[offset]),
			accumulatorData[offset + M1_IDX],
			accumulatorData[offset + M2_IDX],
			accumulatorData[offset + M3_IDX],
			accumulatorData[offset + M4_IDX],
			static_cast<bool>(accumulatorData[offset + INTEGER_ONLY_IDX])
		};
	}

	currentJob->Items = results;
	
}
