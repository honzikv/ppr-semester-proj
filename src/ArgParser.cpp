#include "ArgumentParser.h"

#include <iostream>

#include "Logging.h"

// Type alias
using PlatformWithDevices = std::pair<cl::Platform, std::vector<cl::Device>>;

template <typename T>
auto lowercase(const std::basic_string<T>& s) {
	std::basic_string<T> s2 = s;
	std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
	return s2;
}


inline auto queryClDevices(const std::vector<std::string>& devices) {
	auto deviceNamesFilter = std::unordered_set<std::string>();
	for (const auto& deviceName : devices) {
		deviceNamesFilter.insert(lowercase(deviceName));
	}

	auto result = std::vector<cl::Device>();
	auto platforms = std::vector<cl::Platform>();
	auto platformDevices = std::vector<cl::Device>();

	// Load all platforms
	cl::Platform::get(&platforms);

	// Iterate over each platform
	for (const auto& platform : platforms) {
		platformDevices.clear();
		platform.getDevices(CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU, &platformDevices);
		if (platformDevices.empty()) {
			continue;
		}

		// Iterate over platform devices and find those that match the filter if it is specified
		for (const auto& device : platformDevices) {
			const auto deviceExtensions = lowercase(device.getInfo<CL_DEVICE_EXTENSIONS>());
			const auto deviceName = std::string(device.getInfo<CL_DEVICE_NAME>());

			// Skip if we cant find double precision extension
			if (!devices.empty() && (!deviceExtensions.find(DOUBLE_PRECISION_DEFAULT) || !deviceExtensions.find(
				DOUBLE_PRECISION_AMD))) {
				log(INFO, "Skipping OpenCL device \"" + deviceName + "\" because it doesn't support double precision");
				continue;
			}

			if (deviceNamesFilter.empty()) {
				result.push_back(device);
				continue;
			}

			// Lowercase the name and check if it is in the filter
			const auto deviceNameLower = lowercase(deviceName);
			if (deviceNamesFilter.find(deviceNameLower) != deviceNamesFilter.end()) {
				result.push_back(device);
				// Remove the record from the filter
				deviceNamesFilter.erase(deviceNameLower);
				continue;
			}

			// If it is not log that it is skipped and continue
			log(INFO, "Skipping OpenCL device \"" + deviceName + "\"");
		}
	}

	// Check if all devices were found, if not log this
	if (!deviceNamesFilter.empty()) {
		log(WARNING, "Could not find following OpenCL devices: ");
		for (const auto& deviceName : deviceNamesFilter) {
			std::cout << deviceName << "\n";
		}

		std::cout << "They will not be utilized" << std::endl;
	}

	return result;
}

ProcessingConfig ArgumentParser::processArgs(const int argc, char** argv) const {
	// For argument parsing we use cxxopts
	auto options = cxxopts::Options("PPR Distribution Estimator", "Possible modes: [single_thread, smp, opencl_devices, all]");
	options.add_options()
		("f,file", "Path to the file with distribution (either absolute or relative)",
		 cxxopts::value<std::filesystem::path>())
		("m,mode", "Processing mode", cxxopts::value<std::string>())
		("d,devices", "List of devices to use", cxxopts::value<std::vector<std::string>>())
		("l,list_cl_devices", "List all available OpenCL devices")
		("x,memory_limit", "Max amount of application memory in MB (1GB to 4GB, 1024MB is exactly 1GB)",
		 cxxopts::value<size_t>()->default_value("1024"))
		("b,benchmark", "Runs given mode as benchmark")
		("benchmark_runs", "Number of benchmark runs", cxxopts::value<size_t>()->default_value("10"))
		("o, output_file", "Path to the output file if any", cxxopts::value<std::filesystem::path>())
		("disable_avx2", "Disables AVX2 vectorized instructions")
		("t,watchdog_timeout", "Timeout for watchdog in seconds", cxxopts::value<size_t>()->default_value("5"))
		("h,help", "Print help");

	options.parse_positional({"file", "mode", "devices"});

	cxxopts::ParseResult result;
	try {
		result = options.parse(argc, argv);
	}
	catch (const std::exception&) {
		std::cout << options.help() << std::endl;
		exit(1); // NOLINT(concurrency-mt-unsafe)
	}


	if (result.count("help")) {
		std::cout << options.help() << std::endl;
		exit(0); // NOLINT(concurrency-mt-unsafe)
	}

	if (result.count("list_cl_devices")) {
		std::cout << "Available OpenCL devices: " << "\n";
		const auto clDevices = queryClDevices({});
		for (const auto& clDevice : clDevices) {
			std::cout << "  -\t" << "\"" << clDevice.getInfo<CL_DEVICE_NAME>() << "\"" << std::endl;

		}
		exit(0); // NOLINT(concurrency-mt-unsafe)
	}

	if (argc < 3) {
		std::cout << options.help();
		exit(1); // NOLINT(concurrency-mt-unsafe)
	}

	return validateArgs(result);
}

ProcessingConfig ArgumentParser::validateArgs(const cxxopts::ParseResult& args) const {
	// Check that the file actually exists
	if (args.count("file") < 1) {
		throw std::runtime_error("File path not specified");
	}

	fs::path filePath;
	try {
		filePath = args["file"].as<std::filesystem::path>();
	}
	catch (const std::exception&) {
		throw std::runtime_error("Could not parse file path");
	}

	// Check whether file path actually exists
	if (!fs::exists(filePath)) {
		throw std::runtime_error("File path " + filePath.string() + " does not exist.");
	}

	// Check processing mode
	if (args.count("mode") < 1 && args.count("devices") < 1) {
		throw std::runtime_error("Processing mode not specified");
	}

	// Mode can actually be parsed as device as well
	// Therefore, check if its lowercased version fits anything and if not treat it as device
	const auto modeArg = args.count("mode") > 0 ? args["mode"].as<std::string>() : "opencl_devices";
	const auto modePosArgIsADevice = PROCESSING_MODES_LUT.find(lowercase(modeArg)) == PROCESSING_MODES_LUT.end();
	const auto processingMode = modePosArgIsADevice
		                            ? ProcessingMode::OPENCL_DEVICES
		                            : PROCESSING_MODES_LUT.at(lowercase(modeArg));

	// Check memory limit
	const auto memoryLimit = args.count("memory_limit") > 0
		                         ? args["memory_limit"].as<size_t>() * 1024 * 1024
		                         : DEFAULT_MEMORY_LIMIT;
	if (memoryLimit > MAX_MEMORY_LIMIT) {
		throw std::runtime_error("Memory limit too high, maximum of 4 GB is allowed");
	}
	if (memoryLimit < DEFAULT_MEMORY_LIMIT) {
		throw std::runtime_error("Memory limit too low, minimum of 1 GB is required");
	}

	if (memoryLimit != DEFAULT_MEMORY_LIMIT) {
		log(INFO, "[JOBSCHEDULER] Memory limit is set to " + std::to_string(memoryLimit / 1024 / 1024) + " MB");
	}


	// Benchmark config
	const auto runBenchmark = args.count("benchmark") > 0 ? args["benchmark"].as<bool>() : false;
	const auto nBenchmarkRuns = args.count("benchmark_runs") > 0 ? args["benchmark_runs"].as<size_t>() : 0;

	// Output path
	const auto outputPath = args.count("output_file") > 0 ? args["output_file"].as<std::filesystem::path>() : "";

	// Use AVX2 instructions
	const auto useAvx2 = args.count("disable_avx2") > 0
		                     ? !args["disable_avx2"].as<bool>()
		                     : static_cast<bool>(__ISA_AVAILABLE_AVX2);

	const auto watchdogTimeout = args.count("watchdog_timeout") > 0
		? args["watchdog_timeout"].as<size_t>() * 1000
		: DEFAULT_WATCHDOG_TIMEOUT;

	if (watchdogTimeout > 60000) {
		log(WARNING, "Detected watchdog timeout over 60s: " + std::to_string(watchdogTimeout / 1000) + "s");
	}

	if (processingMode == ProcessingMode::SMP || processingMode == ProcessingMode::SINGLE_THREAD) {
		return {
			processingMode,
			filePath,
			{},
			memoryLimit,
			runBenchmark,
			nBenchmarkRuns,
			outputPath,
			useAvx2,
			watchdogTimeout,
		};
	}

	// Otherwise we have OpenCL devices or ALL mode
	// Query all OpenCL devices and filter them if necessary
	if (processingMode == ProcessingMode::ALL) {
		return {
			processingMode,
			filePath,
			queryClDevices({}),
			memoryLimit,
			runBenchmark,
			nBenchmarkRuns,
			outputPath,
			useAvx2,
			watchdogTimeout,
		};
	}

	// Otherwise we have OpenCL devices mode
	// Check if we have any devices specified - this can be saved in mode as well since we positionally parsed the input
	auto deviceNames = args.count("devices") > 0
		                   ? args["devices"].as<std::vector<std::string>>()
		                   : std::vector<std::string>();

	// Check if mode is a device
	if (modePosArgIsADevice) {
		deviceNames.push_back(modeArg);
	}

	if (deviceNames.empty()) {
		throw std::runtime_error("No OpenCL devices specified");
	}

	const auto clDevices = queryClDevices({deviceNames.begin(), deviceNames.end()});
	if (clDevices.empty()) {
		throw std::runtime_error("No OpenCL devices found");
	}

	return {
		processingMode,
		filePath,
		clDevices,
		memoryLimit,
		runBenchmark,
		nBenchmarkRuns,
		outputPath,
		useAvx2,
		watchdogTimeout,
	};
}
