#pragma once
#include <iostream>
#include <mutex>
#include <ostream>
#include <vector>
#include <ctime>
#include <array>
#include <sstream>

// This simple "module" is used for console logging

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
 * \brief Logs message to the console, message must be of type std::string.
 *		  This must NEVER be called if other mutex is locked
 * \param logSeverity severity of the log
 * \param message const ref to message
 */
inline void log(const LogSeverity logSeverity, const std::string& message) {
	// For reasonable compiler << is threadsafe for cout
	auto stringStream = std::stringstream();
	stringStream << "[" << getCurrentTimeAsStr() << "] " << LOG_TYPE_LUT[logSeverity] << " " << message << std::endl;
	std::cout << stringStream.str();
}

