#pragma once
#include <filesystem>
#include <vector>
#include <CL/cl.hpp>

namespace fs = std::filesystem;
constexpr auto DEFAULT_MEMORY_LIMIT = 1024ULL * 1024 * 1024;

// This is very naive, but we expect that the system actually gives us 4 GB
constexpr auto MAX_MEMORY_LIMIT = 4ULL * 1024 * 1024 * 1024;

enum ProcessingMode {
	// == basic implementation with one thread
	SINGLE_THREAD,
	// multiple threads via OneTBB
	SMP,
	// One or multiple OpenCL deviceNames specified in vector / list
	OPENCL_DEVICES,
	// All available deviceNames
	ALL,
};

struct ProcessingConfig {
	/**
	 * \brief Processing mode of the application
	 */
	ProcessingMode ProcessingMode;

	/**
	 * \brief Filesystem path to the processed path
	 */
	fs::path DistFilePath;

	/**
	 * \brief List of all OpenCL devices
	 */
	std::vector<cl::Device> ClDevices;

	/**
	 * \brief Application memory limit - the app does not allocate more memory than this for reasonable file sizes
	 */
	size_t MemoryLimit = DEFAULT_MEMORY_LIMIT;

	/**
	 * \brief Whether to run the application in benchmark mode
	 */
	bool IsBenchmark;

	/**
	 * \brief Number of runs per benchmark
	 */
	size_t NBenchmarkRuns;

	/**
	 * \brief Filesystem path to the output file to write the results
	 */
	fs::path OutputPath;
};
