#pragma once
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <CL/cl.hpp>

namespace fs = std::filesystem;
constexpr auto DEFAULT_MEMORY_LIMIT = 1024ULL * 1024 * 1024;

// This is very naive, but we expect that the system actually gives us 4 GB
constexpr auto MAX_MEMORY_LIMIT = 4ULL * 1024 * 1024 * 1024;



enum ProcessingMode {
	/**
	 * \brief Single threaded computation
	 */
	SINGLE_THREAD,
	/**
	 * \brief Symmetric multiprocessor computation - i.e. multithreading using all CPU cores
	 */
	SMP,
	// One or multiple OpenCL deviceNames specified in vector / list
	OPENCL_DEVICES,
	// All available deviceNames
	ALL,
};

// LUT for processing modes so we don't have to do 20 if statements
inline const auto PROCESSING_MODES_LUT = std::unordered_map<std::string, ProcessingMode>{
	{"single_thread", ProcessingMode::SINGLE_THREAD},
	{"smp", ProcessingMode::SMP},
	{"opencl_devices", ProcessingMode::OPENCL_DEVICES},
	{"all", ProcessingMode::ALL},
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

	/**
	 * \brief Whether to use AVX2 vector instructions - note that this is actually only used if the binary is compiled for
	 *		  CPU that supports it and is set to true
	 */
	bool UseAvx2Instructions;

	/**
	 * \brief Timeout for watchdog in milliseconds
	 */
	size_t WatchdogTimeoutMs;
};
