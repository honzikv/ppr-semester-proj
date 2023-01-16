#include "ClSources.h"
#include "StatUtils.h"
#include "Logging.h"
#include "ClDeviceCoordinator.h"

auto ClDeviceCoordinator::compile(const std::string& source, const std::string& programName,
                                  const cl::Context& deviceContext) const {
	auto program = cl::Program(deviceContext, source);
	const auto result = program.build(DEFAULT_BUILD_FLAG);
	 auto buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
	if (buildLog.empty() || buildLog == "\n") {
		buildLog = "Compilation successful";
	}
	log(DEBUG, "[OPENCLBUILD - " + deviceName + " " + deviceType + "] " + buildLog);
	if (result != CL_BUILD_SUCCESS) {
		throw ClCompileErr(
			"Error during OpenCL Program compilation ( " + programName + " )\n. Error: " + std::to_string(result));
	}

	return program;
}

ClDeviceCoordinator::ClDeviceCoordinator(const CoordinatorType coordinatorType,
                                         const ProcessingMode processingMode,
                                         const std::function<void(std::unique_ptr<Job>, size_t)>& jobFinishedCallback,
                                         const std::function<void(size_t)>& notifyWatchdogCallback,
                                         const std::function<void(CoordinatorErr)>& errCallback,
                                         const size_t chunkSizeBytes,
                                         const size_t bytesPerAccumulator,
                                         const size_t clHostBufferSizeBytes,
                                         fs::path& distFilePath,
                                         const size_t id,
                                         cl::Device device):
	DeviceCoordinator(
		coordinatorType,
		processingMode,
		jobFinishedCallback,
		notifyWatchdogCallback,
		errCallback,
		chunkSizeBytes,
		bytesPerAccumulator, distFilePath, id),
	device(std::move(device)),
	maxHostChunks(
		clHostBufferSizeBytes / chunkSizeBytes) {
	// Setup the device
	setup();

	// After we have set everything up start the thread
	startCoordinatorThread();
}

void ClDeviceCoordinator::setup() {
	// OpenCL boilerplate
	context = cl::Context(device);
	commandQueue = cl::CommandQueue(context, device);
	program = compile(CL_PROGRAM, "program", context);
	deviceName = device.getInfo<CL_DEVICE_NAME>();
	estimateWorkgroupSize();

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
		                ? static_cast<size_t>(std::floor(
			                static_cast<double>(maxDeviceBufferSize) / static_cast<double>(chunkSizeBytes)))
		                : maxHostChunks;

	// Set the device type
	if (device.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU) {
		this->deviceType = "CPU";
	}
}

void ClDeviceCoordinator::throwIfStatusUnsuccessful(const cl_int clStatus) const {
	if (clStatus != CL_SUCCESS) {
		log(CRITICAL, "OpenCL error occurred. Error: " + std::to_string(clStatus));
		throw std::runtime_error("OpenCL error occurred. Error: " + std::to_string(clStatus));
	}
}

void ClDeviceCoordinator::estimateWorkgroupSize() {
	const auto kernel = cl::Kernel(program, KERNEL_NAME);
	const auto clRecommendedWorkgroupSize = kernel.getWorkGroupInfo<
		CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(device);

	// The same could be done for AMD Radeon GPUs
	const auto vendorName = device.getInfo<CL_DEVICE_VENDOR>();
	const auto clDeviceName = device.getInfo<CL_DEVICE_NAME>();
	if (vendorName.find("NVIDIA") != std::string::npos && (clDeviceName.find("GTX") != std::string::npos ||
		clDeviceName.find("RTX") != std::string::npos)) {
		const auto nvidiaWorkgroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() / 8;
		log(DEBUG, "[OPENCL] Detected NVIDIA RTX/GTX GPU, using larger workgroup size: " + std::to_string(
			    nvidiaWorkgroupSize));

		if (nvidiaWorkgroupSize < clRecommendedWorkgroupSize) {
			maxWorkGroupSize = nvidiaWorkgroupSize;
			return;
		}

		maxWorkGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() / 8;

		return;
	}

	log(DEBUG, "[OPENCL] Non-Nvidia GPU detected, using recommended workgroup size " + std::to_string(
		    clRecommendedWorkgroupSize));
	maxWorkGroupSize = clRecommendedWorkgroupSize;
}

// ReSharper disable once CppMemberFunctionMayBeConst
auto ClDeviceCoordinator::performJobSetup(cl_int& clStatus) {
	// Get total number of accumulators
	auto nAccumulators = currentJob->getNChunks() / chunksPerAccumulator;

	// Get total number of workgroup runs
	auto totalBytes = nAccumulators * bytesPerAccumulator;
	if (nAccumulators == 0 && currentJob->getSizeBytes(chunkSizeBytes) < chunksPerAccumulator * chunkSizeBytes) {
		// Our file is smaller than chunksPerAccumulator - therefore we only run one work item in one workgroup
		nAccumulators = 1;
		totalBytes = currentJob->getSizeBytes(chunkSizeBytes);
	}

	log(DEBUG,
	    "[OPENCL - " + deviceName + " (" + deviceType + ")" + "] Job split into " + std::to_string(nAccumulators) +
	    " accumulators");
	// Create buffer for results
	const auto accumulatorsBuffer = cl::Buffer(context, CL_MEM_READ_WRITE,
	                                           nAccumulators * N_CL_OUT_ITEMS * sizeof(double), nullptr,
	                                           &clStatus);
	throwIfStatusUnsuccessful(clStatus);

	// Write true values for isInteger only
	auto accumulatorData = std::vector<double>(nAccumulators * N_CL_OUT_ITEMS, 0.0);
	for (auto i = 0ULL; i < nAccumulators; i += 1) {
		accumulatorData[i * N_CL_OUT_ITEMS + INTEGER_ONLY_IDX] = static_cast<double>(true);
		accumulatorData[i * N_CL_OUT_ITEMS + MIN_IDX] = std::numeric_limits<double>::infinity();
	}

	clStatus = commandQueue.enqueueWriteBuffer(accumulatorsBuffer, CL_TRUE, 0, accumulatorData.size() * sizeof(double),
	                                           accumulatorData.data());
	throwIfStatusUnsuccessful(clStatus);

	// Create buffer for data
	const auto dataBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, maxHostChunks * chunkSizeBytes, nullptr, &clStatus);
	throwIfStatusUnsuccessful(clStatus);

	return std::make_tuple(nAccumulators, totalBytes, accumulatorsBuffer, dataBuffer, accumulatorData);
}


void ClDeviceCoordinator::onProcessJob() {
	log(INFO, "[OPENCL - " + deviceName + " (" + deviceType + ")" + "] Processing job with id " +
	    std::to_string(currentJob->Id));

	auto clStatus = cl_int{};
	auto [
		nAccumulators,
		totalBytes,
		accumulatorsBuffer,
		dataBuffer,
		accumulatorData
	] = performJobSetup(clStatus);

	// Load the kernel
	auto kernel = cl::Kernel(program, KERNEL_NAME);

	// Run the computation
	auto bytesRemaining = totalBytes;
	auto bytesProcessedPerAccumulator = 0ULL;
	const auto [startIdx, _] = currentJob->ChunkIdxRange;
	while (bytesRemaining > 0) {
		// Load either max size of the buffer if there is too much data to load or the remaining data
		const auto chunksToLoad = bytesRemaining > maxHostChunks * chunkSizeBytes
			                          ? maxHostChunks // buffer size is maxHostChunks * chunkSizeBytes
			                          : bytesRemaining / chunkSizeBytes; // or something smaller

		// Load chunks "into the device" - or rather schedule to do so via OpenCL
		dataLoader.loadChunksIntoDeviceBuffer(nAccumulators, chunksToLoad, startIdx, bytesProcessedPerAccumulator,
		                                bytesPerAccumulator,
		                                dataBuffer, commandQueue);

		// Amount of items is the number of bytes to load divided by the number of accumulators and the size of double (i.e. 8 bytes)
		const auto itemsToProcess = (chunksToLoad * chunkSizeBytes) / nAccumulators / sizeof(double);

		// Pass args to the kernel
		kernel.setArg(0, dataBuffer);
		kernel.setArg(1, accumulatorsBuffer);
		kernel.setArg(2, itemsToProcess);

		// Schedule to execute the kernel
		clStatus = commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(nAccumulators),
		                                             cl::NDRange(nAccumulators));
		throwIfStatusUnsuccessful(clStatus);

		const auto bytesProcessed = chunksToLoad * chunkSizeBytes;
		bytesRemaining -= bytesProcessed;
		bytesProcessedPerAccumulator += bytesProcessed / nAccumulators;
		notifyWatchdogCallback(chunksToLoad * chunkSizeBytes);
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
			static_cast<bool>(accumulatorData[offset + INTEGER_ONLY_IDX]),
			accumulatorData[offset + MIN_IDX],
		};
	}

	currentJob->Items = results;
	log(DEBUG,
	    "[OPENCL - " + deviceName + " (" + deviceType + ")" + "] Finished computing job with id " +
	    std::to_string(currentJob->Id) + ". Computed " + std::to_string(
		    currentJob->getNChunks()) + " chunks. Chunk size is " + std::to_string(chunkSizeBytes) + " bytes");
}
