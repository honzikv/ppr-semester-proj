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
    for (auto& val : randomDoubles) {
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

auto computeExpected(const std::vector<double>& randomDoubles) {
    auto expected = std::vector<double>();
    expected.reserve(randomDoubles.size() - 150);
    for (auto val : randomDoubles) {
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
    std::cout << "Max work group size: " << maxWorkGroupSize << std::endl;

    auto randomDoubles = generateRandomDoubles();
    auto expected = computeExpected(randomDoubles);  // get expected

    auto context = cl::Context(gpu);
    auto program = compile(kernel, "program", context);

}
