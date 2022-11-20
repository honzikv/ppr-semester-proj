#pragma once

#include <CL/cl.hpp>
#include <stdexcept>

auto getNvidiaGpu() {
    auto platforms = std::vector<cl::Platform>();
    cl::Platform::get(&platforms);

    for (auto& platform : platforms) {
        auto devices = std::vector<cl::Device>();
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

        for (auto& device : devices) {
            auto name = std::string();
            device.getInfo(CL_DEVICE_NAME, &name);

            if (name.find("NVIDIA") != std::string::npos) {
                // Print name and return
                std::cout << "Found GPU: " << name << std::endl;
                return std::make_pair(platform, device);
            }
        }
    }


    throw std::runtime_error("No NVIDIA GPU found");
}
