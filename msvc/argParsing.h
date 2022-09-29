#pragma once
#include <filesystem>
#include <iostream>
#include <string>


namespace fs = std::filesystem;
using Args = wchar_t**;

constexpr auto MinArgs = 2; // or 3 if we count the name of the program
constexpr auto DevicesStartIdx = 2; // start index in the argv array

inline std::pair<fs::path, std::vector<std::string>> ParseArgs(const int n_args, const Args& args) {
	if (n_args - 1 < MinArgs) {
		throw std::invalid_argument("Invalid number of arguments provided. Expected two instead got: " +
			std::to_string(n_args)
		);
	}

	// Parse file path
	const auto filePath = fs::path(args[1]);

	// And the CPU / GPU devices
	auto devices = std::vector<std::string>();
	for (auto i = DevicesStartIdx; i < n_args; i += 1) {
		// TODO remove the stupid conversion bug
		const auto wstr = std::wstring(args[i]);
		devices.emplace_back(wstr.begin(), wstr.end()); // convert to standard API
	}

	return {filePath, devices};
}
