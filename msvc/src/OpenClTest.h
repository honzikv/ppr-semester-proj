#pragma once
#include <iostream>
#include <stdexcept>
#include <CL/cl.hpp>
#include <random>

const auto kernel = R"CLC(

	__kernel void processArray(__global double* array, int nItemsToProcess) {
    size_t threadIdx = get_global_id(0);
    for (size_t i = 0; i < nItemsToProcess; i += 1) {
		array[threadIdx*nItemsToProcess+i] = threadIdx;
    }
}

#define EXPONENT_MASK 0x7fffffffffffffffULL
#define MANTISSA_MASK 0x000fffffffffffffULL

typedef ulong uint64_t;

// Modification of std::fpclassify
// Returns true if the number is FP_NORMAL or FP_ZERO
inline bool valueNormalOrZero(double x) {
    uint64_t bits = *(uint64_t*)&x;
    uint64_t exponent = (bits & EXPONENT_MASK) >> 52;

    if (exponent == 0) {
        if (bits & MANTISSA_MASK) {
            // Denormal
            return false;
        }

        // Zero
        return true;
    }

    if (exponent == 0x7ff) {
        // Infinity or NaN
        return false;
    }

    // normal
    return true;
}

__kernel void computeStats(__global double* buffer, uint64_t numElements) {
    size_t threadIdx = get_global_id(0);

    // Emulate a RunningStats object
    uint64_t n = 0;
    bool integerOnly = true;
    double m1 = 0.0, m2 = 0.0, m3 = 0.0, m4 = 0.0;
    double intPart = 0.0;

    // Process numElements elements
    for (uint64_t i = 0; i < numElements; i += 1) {
        double x = buffer[threadIdx*numElements+i];
        if (!valueNormalOrZero(x)) {
            // Skip if x is NaN, infinity or denormal
            continue;
        }

        n += 1;
        integerOnly = integerOnly && modf(x, &intPart) == 0.0;
        double delta = x - m1;
        double deltaN = delta / n;
        double deltaNSquared = deltaN * deltaN;
        double term1 = delta * deltaN * (n - 1);

        m1 += deltaN;
        m4 += term1 * deltaNSquared * (n * n - 3 * n + 3) + 6 * deltaNSquared * m2 - 4 * deltaN * m3;
        m3 += term1 * deltaN * (n - 2) - 3 * deltaNSquared * m2;
        m2 += term1;
    }
    
    // Write results to the buffer
    buffer[threadIdx*6] = n;
    buffer[threadIdx*6 + 1] = m1;
    buffer[threadIdx*6 + 2] = m2;
    buffer[threadIdx*6 + 3] = m3;
    buffer[threadIdx*6 + 4] = m4;
    buffer[threadIdx*6 + 5] = integerOnly;
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

    const auto count = 20 * 1024 * 1024;
	auto data = std::vector<double>(count);
    std::random_device rd;

    std::mt19937 e2(rd());

    std::normal_distribution<> dist(100, 50);
	// Fill data with random values
	for (auto& value : data) {
		value = dist(e2);
	}

    auto nWorkers = 8;
    const auto dataPerWorker = count / nWorkers;


	auto program = cl::Program(context, cl::Program::Sources{ 1, std::make_pair(kernel, strlen(kernel)) });
    program.build("-cl-std=CL2.0");
	auto queue = cl::CommandQueue(context, device);

	auto kernel = cl::Kernel(program, "computeStats");

	auto output = std::vector<double>(count);

	// Write data to device
	auto deviceInputBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(double) * count);
	queue.enqueueWriteBuffer(deviceInputBuffer, CL_TRUE, 0, sizeof(double) * count / 2, data.data());
	queue.enqueueWriteBuffer(deviceInputBuffer, CL_TRUE, sizeof(double) * count / 2, sizeof(double) * count / 2, data.data());
	// auto deviceOutputBuffer = cl::Buffer(context, , sizeof(double) * count);

	kernel.setArg(0, deviceInputBuffer);
	// kernel.setArg(1, deviceOutputBuffer);
    kernel.setArg(1, static_cast<size_t>(dataPerWorker));

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
