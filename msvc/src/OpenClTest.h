#pragma once
#include <iostream>
#include <stdexcept>
#include <CL/cl.hpp>

const auto kernel = R"CLC(

	__kernel void processArray(__global double* array, int nItemsToProcess) {
    size_t threadIdx = get_global_id(0);
    for (size_t i = 0; i < nItemsToProcess; i += 1) {
		array[threadIdx*nItemsToProcess+i] = threadIdx;
    }
}

)CLC";

auto getRtx3060ti() {
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

void openClTest() {
    auto [_, device] = getRtx3060ti();

	auto context = cl::Context(device);

    const auto count = 1000;
    const auto dataPerWorker = 5;
	auto data = std::vector<double>(count, 2.0);
    auto nWorkers = count / dataPerWorker;

	auto program = cl::Program(context, cl::Program::Sources{ 1, std::make_pair(kernel, strlen(kernel)) });
    program.build({device});
	auto queue = cl::CommandQueue(context, device);

	auto kernel = cl::Kernel(program, "processArray");

	auto output = std::vector<double>(count);

	// Write data to device
	auto deviceInputBuffer = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(double) * count, data.data());
	// auto deviceOutputBuffer = cl::Buffer(context, , sizeof(double) * count);

	kernel.setArg(0, deviceInputBuffer);
	// kernel.setArg(1, deviceOutputBuffer);
	kernel.setArg(1, dataPerWorker);

	queue.enqueueNDRangeKernel(kernel, cl::NullRange, nWorkers);
	queue.enqueueReadBuffer(deviceInputBuffer, CL_TRUE, 0, sizeof(double) * count, output.data());

	for (auto i = 0; i < count; i += 1) {
		std::cout << output[i] << ", ";
		if (i % dataPerWorker == dataPerWorker - 1) {
			std::cout << std::endl;
		}
	}
    std::cout << std::endl;
}
