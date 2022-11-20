#pragma once

#include <regex>
#include <string>
#include <vector>
#include <filesystem>
#include <CL/cl.hpp>

template <typename T>
auto lowercase(const std::basic_string<T>& s) {
	std::basic_string<T> s2 = s;
	std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
	return s2;
}

namespace fs = std::filesystem;

// Type alias
using PlatformWithDevices = std::pair<cl::Platform, std::vector<cl::Device>>;

/**
 * \brief Structure containing the parsed arguments
 */
struct ProcessingArgs {
	std::string ProcessingMode;
	std::string DistFilePath; // Path to the file with the distribution
	std::vector<std::string> DeviceNames;
};

static const auto QUOTES_REGEX = std::regex("\"([^\"]*)\"");
static const auto OPENCL_DEVICES_STR = "opencldevices";

/**
 * \brief Parses the command line arguments
 * \param argc number of arguments
 * \param argv array of arguments
 */
auto parseArguments(const int argc, char** argv) -> ProcessingArgs;

enum ProcessingMode {
	SINGLE_THREAD,
	// == basic implementation with one thread
	SMP,
	// multiple threads via OneTBB
	OPENCL_DEVICES,
	// One or multiple OpenCL deviceNames specified in vector / list
	ALL // All available deviceNames
};

struct ProcessingInfo {
	ProcessingMode ProcessingMode;
	fs::path DistFilePath;
	std::vector<PlatformWithDevices> Devices;
};

/**
 * \brief Validates arguments and returns ProcessingInfo object if successful, terminates program otherwise
 * \param args parsed arguments
 * \return processing info object if validation succeeds, terminates program otherwise
 */
auto validateArguments(ProcessingArgs args) -> ProcessingInfo;
