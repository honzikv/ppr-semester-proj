#pragma once
#include <filesystem>
#include <vector>
#include <CL/cl.hpp>

namespace fs = std::filesystem;
constexpr auto DEFAULT_MEMORY_LIMIT = 1024ULL * 1024 * 1024;

// This is very naive, but we expect that the system actually gives us 4 GB
constexpr auto MAX_MEMORY_LIMIT = 4ULL * 1024 * 1024 * 1024;
enum ProcessingMode {
	SINGLE_THREAD,
	// == basic implementation with one thread
	SMP,
	// multiple threads via OneTBB
	OPENCL_DEVICES,
	// One or multiple OpenCL deviceNames specified in vector / list
	ALL // All available deviceNames
};

struct ProcessingConfig {
	ProcessingMode ProcessingMode;
	fs::path DistFilePath;
	std::vector<cl::Device> Devices;
	size_t MemoryLimit = DEFAULT_MEMORY_LIMIT;
};
