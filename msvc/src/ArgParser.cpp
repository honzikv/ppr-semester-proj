#include "ArgParser.h"

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

// LUT for processing modes so we don't have to do 20 if statements
const auto processingModesLut = std::unordered_map<std::string, ProcessingMode>{
	{"single_thread", ProcessingMode::SINGLE_THREAD},
	{"smp", ProcessingMode::SMP},
	{"opencl_devices", ProcessingMode::OPENCL_DEVICES},
	{"all", ProcessingMode::ALL},
};


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
			if (!deviceExtensions.find(DOUBLE_PRECISION_DEFAULT) || !deviceExtensions.find(DOUBLE_PRECISION_AMD)) {
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

ProcessingConfig ArgumentParser::processArgs(int argc, char** argv) const {
	// For argument parsing we use cxxopts
	auto options = cxxopts::Options("PPR Distribution Estimator", "A simple distribution estimator tool");
	options.add_options()
		("f, file", "Path to the file with distribution (either absolute or relative)",
		 cxxopts::value<std::filesystem::path>())
		("m, mode", "Processing mode", cxxopts::value<std::string>())
		("d, devices", "List of devices to use", cxxopts::value<std::vector<std::string>>())
		("memory", "Memory limit in MBs - defaults to 1024.",
		 cxxopts::value<size_t>()->default_value("1024"))
		("h, help", "Print help");

	if (argc < 3) {
		std::cout << options.help();
		exit(1);
	}

	options.parse_positional({"file", "mode", "devices"});
	const auto result = options.parse(argc, argv);

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
	catch (const std::exception& ) {
		throw std::runtime_error("Could not parse file path");
	}

	// Check whether file path actually exists
	if (!fs::exists(filePath)) {
		throw std::runtime_error("File path" + filePath.string() + " does not exist.");
	}

	// Check processing mode
	if (args.count("mode") < 1) {
		throw std::runtime_error("Processing mode not specified");
	}

	// Mode can actually be parsed as device as well
	// Therefore, check if its lowercased version fits anything and if not treat it as device
	const auto modeArg = args["mode"].as<std::string>();
	const auto modePosArgIsADevice = processingModesLut.find(lowercase(modeArg)) == processingModesLut.end();
	const auto processingMode = modePosArgIsADevice
		? ProcessingMode::OPENCL_DEVICES
		: processingModesLut.at(modeArg);

	// Check memory limit
	const auto memoryLimit = args.count("memory") > 0
		? args["memory"].as<size_t>() * 1024 * 1024
		: DEFAULT_MEMORY_LIMIT;
	if (memoryLimit > MAX_MEMORY_LIMIT) {
		throw std::runtime_error("Memory limit too high, maximum of 4 GB is allowed");
	}
	if (memoryLimit < DEFAULT_MEMORY_LIMIT) {
		throw std::runtime_error("Memory limit too low, minimum of 1 GB is required");
	}

	if (processingMode == ProcessingMode::SMP || processingMode == ProcessingMode::SINGLE_THREAD) {
		return { processingMode, filePath, {}, memoryLimit };
	}

	// Otherwise we have OpenCL devices or ALL mode
	// Query all OpenCL devices and filter them if necessary
	if (processingMode == ProcessingMode::ALL) {
		return { processingMode, filePath, queryClDevices({}), memoryLimit };
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

	const auto clDevices = queryClDevices({ deviceNames.begin(), deviceNames.end() });
	if (clDevices.empty()) {
		throw std::runtime_error("No OpenCL devices found");
	}

	return { processingMode, filePath, clDevices, memoryLimit };
}
