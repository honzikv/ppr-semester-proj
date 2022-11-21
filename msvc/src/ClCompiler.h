#pragma once
#include <stdexcept>
#include <CL/cl.hpp>
#include <string>

constexpr auto DEFAULT_BUILD_FLAG = "-cl-std=CL2.0";

/**
 * \brief Custom error for control flow
 */
class ClCompileErr final : public std::runtime_error {
public:
	explicit ClCompileErr(const std::string& what = "") : std::runtime_error(what) {
	}
};

/**
 * \brief Compiles given source into program
 * \param source string containing source code to be compiled
 * \param programName name of the program
 * \param deviceContext device context
 * \param device device to compile for
 * \return cl::Program instance or throws ClCompileErr if the program cannot be compiled
 */
inline auto compile(const std::string& source, const std::string& programName, const cl::Context& deviceContext) {
	const auto program = cl::Program(deviceContext, source);
	if (const auto result = program.build(DEFAULT_BUILD_FLAG); result != CL_BUILD_SUCCESS) {
		throw ClCompileErr(
			"Error during OpenCL Program compilation ( " + programName + " )\n. Error: " + std::to_string(result));
	}

	return program;
}
