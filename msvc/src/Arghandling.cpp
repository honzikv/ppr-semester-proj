#include "Arghandling.h"
#include <iostream>
#include <ostream>
#include <unordered_map>
#include <utility>
#include <unordered_set>

#include "ConcurrencyUtils.h"


// LUT for processing modes so we don't have to do 20 if statements
const auto processingModes = std::unordered_map<std::string, ProcessingMode>{
	{"singlethread", ProcessingMode::SINGLE_THREAD},
	{"smp", ProcessingMode::SMP},
	{"opencldevices", ProcessingMode::OPENCL_DEVICES},
	{"all", ProcessingMode::ALL},
};

auto printValidModes() {
	std::cout << "Processing modes: " << '\n';
	std::cout << "  singleThread - single thread mode" << '\n';
	std::cout << "  all - SMP + OpenCL" << '\n';
	std::cout << "  SMP - SMP" << '\n';
	std::cout << "  <list of OpenCL deviceNames>" << std::endl;
}

auto parseArguments(const int argc, char** argv) -> ProcessingArgs {
	// Two arguments are required - path to the distribution file and the processing mode
	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " <path to distribution> <mode>" << std::endl;
		printValidModes();
		exit(1); // NOLINT(concurrency-mt-unsafe) - this is whatever because start is always single threaded
	}

	// Parse the arguments
	const auto distFilePath = std::string(argv[1]);

	// If there are three arguments and third argument doesnt start with " we just parse the mode and return it
	if (argc == 3 && argv[2][0] != '"') {
		const auto mode = std::string(argv[2]);
		return {mode, distFilePath, {}};
	}

	// TODO it parses strings by default so I guess ignore this - use this if not working in release
	// Otherwise extract the deviceNames
	// auto deviceNames = std::vector<std::string>();

	// Take all arguments after the first two
	// auto deviceListStr = std::string();
	// for (auto i = 2; i < argc; i += 1) {
	// 	std::cout << argv[i] << std::endl;
	// 	deviceListStr.append(argv[i]);
	// }

	// Now extract devices which are in quotes
	// for (auto iter = std::sregex_iterator(deviceListStr.begin(), deviceListStr.end(), QUOTES_REGEX);
	//      iter != std::sregex_iterator();
	//      ++iter) {
	// 	const auto matchStr = (*iter).str();
	// 	deviceNames.push_back(matchStr.substr(1, matchStr.size() - 2));
	// }

	auto deviceNames = std::vector<std::string>();
	for (auto i = 2; i < argc; i += 1) {
		deviceNames.push_back(argv[i]);
	}

	// Return the result
	return {OPENCL_DEVICES_STR, distFilePath, deviceNames};
}

auto loadAllClDevices() {
	auto result = std::vector<PlatformWithDevices>();
	auto platforms = std::vector<cl::Platform>();
	auto platformDevices = std::vector<cl::Device>();
	cl::Platform::get(&platforms);
	for (const auto& platform : platforms) {
		platformDevices.clear();
		platform.getDevices(CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU, &platformDevices);
		if (platformDevices.empty()) {
			continue;
		}

		// Otherwise add record to the result
		result.push_back({platform, platformDevices});
	}

	return result;
}

/**
 * \brief Finds all relevant OpenCL devices according to requestedDevices filter
 * \param requestedDevices set of all requested devices
 * \param foundDevices vector where found devices with their respective platform are added
 * \param foundDevicesCount counter of all found devices
 * \param platform current platform
 * \param devices devices for current platform
 * \return None
 */
auto findRelevantClDevices(const std::unordered_set<std::string>& requestedDevices,
                           std::vector<PlatformWithDevices>& foundDevices,
                           size_t& foundDevicesCount,
                           const cl::Platform& platform,
                           const std::vector<cl::Device>& devices) {
	auto platformWithDevices = PlatformWithDevices(platform, {});
	for (const auto& device : devices) {
		auto deviceName = std::string();
		device.getInfo(CL_DEVICE_NAME, &deviceName);

		// Skip if the name does not exist
		if (requestedDevices.find(deviceName) == requestedDevices.end()) {
			continue;
		}

		// Otherwise add device to the platform
		platformWithDevices.second.push_back(device);
	}

	// If the platform has some devices add it to the list
	if (!platformWithDevices.second.empty()) {
		foundDevices.push_back(platformWithDevices);
		foundDevicesCount += 1;
	}
}

auto validateArguments(ProcessingArgs args) -> ProcessingInfo {
	// First check that the file exists
	const auto distFilePath = fs::path(args.DistFilePath);
	if (!exists(distFilePath)) {
		std::cout << "File path: \"" << distFilePath.string() << "\" does not exist." << std::endl;
		exit(1);
	}

	// Validate processing mode
	const auto processingModeStr = Utils::lowercase(args.ProcessingMode);
	if (processingModes.find(processingModeStr) == processingModes.end()) {
		std::cout << "Invalid processing mode: \"" << args.ProcessingMode << "\"." << std::endl;
		printValidModes();
		exit(1); // NOLINT(concurrency-mt-unsafe)
	}

	const auto processingMode = processingModes.at(processingModeStr);
	if (processingMode == ProcessingMode::SINGLE_THREAD || processingMode == ProcessingMode::SMP) {
		// Return the object as we won't be loading any other devices
		return {processingMode, distFilePath, {}};
	}

	// Otherwise load all opencl devices
	const auto allClDevices = loadAllClDevices();

	// If there are no devices available log error
	if (allClDevices.empty()) {
		std::cout << "No OpenCL devices were found" << std::endl;
		exit(1); // NOLINT(concurrency-mt-unsafe)
	}

	if (processingMode == ProcessingMode::ALL) {
		return {processingMode, distFilePath, allClDevices};
	}

	// Otherwise filter only specific devices
	const auto requestedDevices = std::unordered_set<std::string>(args.DeviceNames.begin(), args.DeviceNames.end());
	auto foundDevices = std::vector<PlatformWithDevices>();
	size_t foundDevicesCount = 0;
	for (const auto& [platform, devices] : allClDevices) {
		findRelevantClDevices(requestedDevices, foundDevices, foundDevicesCount, platform, devices);
	}

	if (foundDevicesCount == 0) {
		std::cout << "Error, no OpenCL devices with specified names were found." << std::endl;
		exit(1); // NOLINT(concurrency-mt-unsafe)
	}

	// Finally if there is less devices than requested throw an error
	if (foundDevicesCount < requestedDevices.size()) {
		std::cout << "Error, missing some OpenCL devices." << std::endl;
		exit(1); // NOLINT(concurrency-mt-unsafe)
	}

	return {processingMode, distFilePath, foundDevices};
}
