#pragma once

#include <regex>
#include <string>
#include <vector>
#include <filesystem>
#include <CL/cl.hpp>

namespace fs = std::filesystem;

// Type alias
using PlatformWithDevices = std::pair<cl::Platform, std::vector<cl::Device>>;

/**
 * \brief Structure containing the parsed arguments
 */
struct ProcessingArgs {
	std::string processingMode;
	std::string distFilePath; // Path to the file with the distribution
	std::vector<std::string> deviceNames;
};

const auto QUOTES_REGEX = std::regex("\"([^\"]*)\"");
const auto OPENCL_DEVICES = "opencldevices";

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
	OPEN_CL_DEVICES,
	// One or multiple OpenCL deviceNames specified in vector / list
	ALL // All available deviceNames
};

struct ProcessingInfo {
	ProcessingMode processingMode;
	fs::path distFilePath;
	std::vector<PlatformWithDevices> devices;
};

/**
 * \brief Validates arguments and returns ProcessingInfo object if successful, terminates program otherwise
 * \param args parsed arguments
 * \return processing info object if validation succeeds, terminates program otherwise
 */
auto validateArguments(ProcessingArgs args) -> ProcessingInfo;
