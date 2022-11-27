#pragma once
#include <iostream>
#include <mutex>
#include <ostream>
#include <vector>
#include <ctime>
#include <array>

// This simple "module" is used for console logging

// Mutex for synchronization - this should not be manipulated with from outside
inline auto LoggerMutex = std::mutex{};

/**
 * \brief Log severity - basically just maps to a string (Info), (Debug), etc...
 */
enum LogSeverity {
	INFO = 0,
	DEBUG = 1,
	WARNING = 2,
	CRITICAL = 3,
};

const auto LOG_TYPE_LUT = std::vector<std::string>{"(Info)", "(Debug)", "(Warning)", "(Error)"};

inline auto getCurrentTimeAsStr() {
	const auto now = time(nullptr);
	auto timeStruct = tm{};
	auto buf = std::array<char, 80>();

	// Load the time
	localtime_s(&timeStruct, &now); // NOLINT(cert-err33-c)

	// Format the timestamp
	strftime(buf.data(), sizeof(buf), "%X", &timeStruct);  // NOLINT(cert-err33-c)

	return std::string(buf.data());
}

/**
 * \brief Logs message to the console, message must be of type std::string
 * \param logSeverity severity of the log
 * \param message const ref to message
 */
inline void log(const LogSeverity logSeverity, const std::string& message) {
	const auto lock = std::scoped_lock(LoggerMutex);
	std::cout << "[" << getCurrentTimeAsStr() << "] " << LOG_TYPE_LUT[logSeverity] << " " << message << std::endl;
}

