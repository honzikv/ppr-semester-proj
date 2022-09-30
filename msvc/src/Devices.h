#pragma once
#include <stdexcept>
#include <vector>
#include <CL/cl.hpp>


inline std::vector<cl::Device> FindAllAvailableDevices() {
	// Load platforms
	auto allPlatforms = std::vector<cl::Platform>();
	cl::Platform::get(&allPlatforms);
	if (allPlatforms.empty()) {
		throw std::invalid_argument("Error, platform does not support OpenCL.");
	}


	// Load available devices
	auto allDevices = std::vector<cl::Device>();
	allPlatforms[0].getDevices(CL_DEVICE_TYPE_ALL, &allDevices);
	if (allDevices.empty()) {
		throw std::invalid_argument("Error, no devices are available for this platform.");
	}

	auto usableDevices = std::vector<cl::Device>();
	for (const auto& device : allDevices) {
        const auto deviceType = device.getInfo<CL_DEVICE_TYPE>();
		if (
			deviceType == CL_DEVICE_TYPE_GPU || deviceType == CL_DEVICE_TYPE_CPU) {
			usableDevices.push_back(device);
		}
	}

	return usableDevices;
}
