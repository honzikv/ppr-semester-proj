#pragma once
#include <iostream>
#include <mutex>
#include <ostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <array>

// Simple logging module

inline auto mutex = std::mutex{};
const auto TIME_FORMAT = "%Y-%m-%d %H:%M:%S";
const auto MS_FORMAT = ".%03d";

enum LogSeverity {
	INFO = 0,
	DEBUG = 1,
	CRITICAL = 2
};

const auto LOG_TYPE_LUT = std::vector<std::string>{"(Info)", "(Debug)", "(Error)"};

inline auto getCurrentTimeAsStr() {
	const auto now = time(nullptr);
	auto timeStruct = tm{};
	auto buf = std::array<char, 80>();

	// I really care about return values
	auto _ = localtime_s(&timeStruct, &now);
	auto __ = strftime(buf.data(), sizeof(buf), "%X", &timeStruct);

	return std::string(buf.data());
}

inline void log(const LogSeverity logSeverity, const std::string& message) {
	const auto lock = std::scoped_lock(mutex);
	std::cout << "[" << getCurrentTimeAsStr() << "] " << LOG_TYPE_LUT[logSeverity] << " " << message << std::endl;
}

