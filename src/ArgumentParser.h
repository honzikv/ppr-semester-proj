#pragma once

#include "ProcessingConfig.h"
#include "include/cxxopts.h"

constexpr auto DOUBLE_PRECISION_DEFAULT = "cl_khr_fp64";
constexpr auto DOUBLE_PRECISION_AMD = "cl_amd_fp64";
constexpr auto DEFAULT_WATCHDOG_TIMEOUT = 5000; // 5 seconds

/**
 * \brief Simple class to parse and validate arguments
 */
class ArgumentParser {
public:
	/**
	 * \brief Processes arguments and returns ProcessConfig or throws std::runtime_error
	 * \param argc argc
	 * \param argv argv 
	 * \return ProcessConfig
	 */
	ProcessingConfig processArgs(int argc, char** argv) const;

private:
	/**
	 * \brief Validates parsed arguments
	 * \param args parsed arguments
	 * \return ProcessingConfig instance
	 */
	[[nodiscard]] ProcessingConfig validateArgs(const cxxopts::ParseResult& args) const;
};
