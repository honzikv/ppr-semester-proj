#pragma once
#include <filesystem>

#include "ClassificationRun.h"
#include "Logging.h"
#include "ProcessingConfig.h"
#include "FilesystemUtils.h"

constexpr auto MAX_RUNS = 1024; // More would be somewhat pointless
constexpr auto DEFAULT_RUNS = 16;

/**
 * \brief Performs one benchmark run of the classification for given processing config
 * \param processingConfig processing configuration
 * \param outputStream vector of output streams to write the results to
 * \return computation time
 */
inline auto performBenchmarkRun(ProcessingConfig& processingConfig) {
	// Configure TBB if needed
	auto tbbThreadControl = tbb::global_control(tbb::global_control::max_allowed_parallelism,
	                                            processingConfig.ProcessingMode == SINGLE_THREAD
		                                            ? 1
		                                            : tbb::this_task_arena::max_concurrency()
	);
	
	auto jobScheduler = JobScheduler(processingConfig);

	// Perform the run - note that only the actual computing time is measured
	auto timer = Timer();
	timer.start();
	const auto result = jobScheduler.run();
	timer.stop();
	classifyDistribution(StatUtils::mergeLeftToRight(result));
	timer.printResults();

	return timer.getElapsedTimeMillis();
}

inline void logStat(const std::string& statName,
                    const std::chrono::duration<int64_t, std::milli> durationMillis,
                    std::ofstream& file,
                    const bool logToFile) {
	const auto millis = durationMillis.count();
	const auto secs = static_cast<double>(millis) / 1000.0;

	// This code is really disgusting but it is easier than doing some string manipulations and streams
	if (statName.empty()) {
		std::cout << "Run durationMillis: " << StatUtils::doubleToStr(secs, 5) << "s (" <<
			millis << "ms)" << std::endl;
		if (logToFile) {
			file << "Run durationMillis: " << StatUtils::doubleToStr(secs, 5) << "s (" <<
				millis << "ms)" << std::endl;
		}
		return;
	}

	std::cout << statName << " run durationMillis: " << StatUtils::doubleToStr(secs, 5) << "s (" <<
		millis << "ms)" << std::endl;
	if (logToFile) {
		file << statName << " run durationMillis: " << StatUtils::doubleToStr(secs, 5) << "s (" <<
			millis << "ms)" << std::endl;
	}
}

inline void setupOutputFileDirsIfNeeded(const ProcessingConfig& config) {
	if (config.OutputPath.empty()) {
		log(INFO, "Output path not specified, the results will only be logged into stdout");
	}
	else {
		// Create output file directories if needed
		const auto parentPath = config.DistFilePath.parent_path();
		try {
			FilesystemUtils::makeDirs(parentPath); // This will throw if it is not possible
		}
		catch (std::runtime_error& err) {
			// Cannot continue if we have nowhere to write the results
			log(CRITICAL, err.what());
			exit(1); // NOLINT(concurrency-mt-unsafe)
		}
	}
}

inline auto getNRuns(const ProcessingConfig& config) {
	auto nRuns = config.NBenchmarkRuns;
	if (nRuns > MAX_RUNS) {
		log(WARNING, "Too many benchmark runs, capping at " + std::to_string(MAX_RUNS));
		nRuns = MAX_RUNS;
	}
	else if (nRuns == 0) {
		log(WARNING, "No benchmark runs specified, defaulting to " + std::to_string(DEFAULT_RUNS));
		nRuns = MAX_RUNS;
	}
	else {
		log(INFO, "Running benchmark with " + std::to_string(nRuns) + " runs");
	}

	return nRuns;
}

/**
 * \brief Runs benchmark for given configuration
 * \param config processing configuration
 */
inline void runBenchmark(ProcessingConfig& config) {
	setupOutputFileDirsIfNeeded(config);
	const auto nRuns = getNRuns(config);

	// Flip PROCESSING_MODES_LUT_TABLE
	auto inverseProcessingModeLutTable = std::unordered_map<ProcessingMode, std::string>();
	for (const auto& [key, value] : PROCESSING_MODES_LUT) {
		inverseProcessingModeLutTable[value] = key;
	}
	log(INFO, "Benchmark will run in mode: " + inverseProcessingModeLutTable[config.ProcessingMode]);

	// Run the benchmark
	log(INFO, "Starting the benchmark\n----------------------");
	auto runDurations = std::vector<std::chrono::duration<long long, std::milli>>();
	try {
		for (auto i = 1ULL; i <= nRuns; i += 1) {
			log(INFO, "Starting run #" + std::to_string(i));
			runDurations.push_back(performBenchmarkRun(config));
		}
	}
	catch (const std::runtime_error& err) {
		log(CRITICAL, err.what());
		exit(1); // NOLINT(concurrency-mt-unsafe)
	}

	// Compute average, best, and worst run time
	const auto averageRunTime =
		std::accumulate(runDurations.begin(), runDurations.end(), std::chrono::milliseconds(0)) /
		runDurations.size();
	const auto maxRunTime = *std::max_element(runDurations.begin(), runDurations.end());
	const auto minRunTime = *std::min_element(runDurations.begin(), runDurations.end());

	const auto logToFile = !config.OutputPath.empty();
	std::ofstream file;
	if (logToFile) {
		file.open(config.OutputPath);
	}

	// Log the results
	log(INFO, "Benchmark results\n-----------------");
	for (const auto& runDuration : runDurations) {
		// Most reasonable time for measurements is a resolution of milliseconds
		logStat("", runDuration, file, logToFile);
	}

	// Log the average, best, and worst run time
	logStat("Average", averageRunTime, file, logToFile);
	logStat("Best", minRunTime, file, logToFile);
	logStat("Worst", maxRunTime, file, logToFile);

}
