#pragma once

#include <iostream>
#include <random>
#include "gpu.h"
#include "ClKernels.h"


auto generateRandomDoubles() {
    // Generate random array where some numbers are NaNs, Infinite, Denormal while most are normal or zero
    std::vector<double> randomDoubles(1024 * 1024);

    auto randomDevice = std::random_device();
    auto randomEngine = std::mt19937(randomDevice());

    auto randomDouble = std::uniform_real_distribution<double>(-100000.0, 100000.0);
    for (auto &val: randomDoubles) {
        val = randomDouble(randomEngine);
    }

    // Add 50 Nans to the beginning
    for (auto i = 0; i < 50; i++) {
        randomDoubles[i] = std::numeric_limits<double>::quiet_NaN();
    }

    // Add 50 after that
    for (auto i = 50; i < 100; i++) {
        randomDoubles[i] = std::numeric_limits<double>::infinity();
    }

    // Add 50 denormals after that
    for (auto i = 100; i < 150; i++) {
        randomDoubles[i] = std::numeric_limits<double>::denorm_min();
    }

    // Shuffle
    std::shuffle(randomDoubles.begin(), randomDoubles.end(), randomEngine);

    return randomDoubles;
}

auto computeExpected(const std::vector<double> &randomDoubles) {
    auto expected = std::vector<double>();
    expected.reserve(randomDoubles.size() - 150);
    for (auto val: randomDoubles) {
        if (std::isnormal(val) || val == 0.0) {
            expected.push_back(val);
        }
    }

    return expected;
}

void testClKernel() {
    auto [platform, gpu] = getNvidiaGpu();

    // Print max work group size
    size_t maxWorkGroupSize;
    gpu.getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &maxWorkGroupSize);
    std::cout << "Used workgroup size: " << maxWorkGroupSize << std::endl;

    auto randomDoubles = generateRandomDoubles();
    auto expected = computeExpected(randomDoubles);  // get expected

    auto context = cl::Context(gpu);

    // Wow it actually compiles on Nvidia
//    auto program = ompilec(colonel, "program", context);

    auto program = compile(testKernel, "program", context);
    auto queue = cl::CommandQueue(context, gpu);

    auto nWorkers = 4;
    auto elementsPerWorker = 4;
    auto nWorkItems = nWorkers * elementsPerWorker;
    auto readBuffer = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, nWorkItems * sizeof(double));
    auto writeBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, nWorkItems * sizeof(double));

    auto hostData = std::vector<double>(nWorkItems, -69.0);

    queue.enqueueWriteBuffer(readBuffer, CL_TRUE, 0, nWorkItems * sizeof(double), hostData.data());

    auto colonel = cl::Kernel(program, "processArray");
    colonel.setArg(0, readBuffer);
    colonel.setArg(0, writeBuffer);
    colonel.setArg(2, elementsPerWorker);

    queue.enqueueNDRangeKernel(colonel, cl::NullRange, nWorkItems);

    // Read back
    auto result = queue.enqueueReadBuffer(writeBuffer, CL_TRUE, 0, nWorkItems * sizeof(double), hostData.data());
    std::cout << "Result: " << result << std::endl;

    // Print results
    for (auto i = 0; i < nWorkers; i += 1) {
        std::cout << hostData[i * elementsPerWorker] << std::endl;
    }
}
