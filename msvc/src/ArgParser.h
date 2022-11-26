#pragma once

#include "ProcessingConfig.h"
#include "../include/cxxopts.h"

constexpr auto DOUBLE_PRECISION_DEFAULT = "cl_khr_fp64";
constexpr auto DOUBLE_PRECISION_AMD = "cl_amd_fp64";

/**
 * \brief Simple class used to parse and validate arguments
 */
class ArgumentParser {
public:
	ProcessingConfig processArgs(int argc, char** argv) const;

	[[nodiscard]] ProcessingConfig validateArgs(const cxxopts::ParseResult& args) const;
};
