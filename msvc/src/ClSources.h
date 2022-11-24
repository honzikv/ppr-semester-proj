#pragma once

#include <string>
#include <CL/cl.hpp>
#include <stdexcept>

constexpr auto CL_PROGRAM = R"CLC(
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
    size_t n = 0;
    bool integerOnly = true;
    double m[4]; // m1, m2, m3, m4
    double intPart;

    // Process numElements elements
    for (size_t i = 0; i < numElements; i += 1) {
        double x = buffer[i];
        // if (!valueNormalOrZero(x)) {
        //     // Skip if x is NaN, infinity or denormal
        //     continue;
        // }

        n += 1;
        integerOnly = integerOnly && modf(x, &intPart) == 0.0;
        double delta = x - m[0];
        double deltaN = delta / n;
        double deltaNSquared = deltaN * deltaN;
        double term1 = delta * deltaN * (n - 1);

        m[0] += deltaN;
        m[3] += term1 * deltaNSquared * (n * n - 3 * n + 3) + 6 * deltaNSquared * m[1] - 4 * deltaN * m[2];
        m[2] += term1 * deltaN * (n - 2) - 3 * deltaNSquared * m[1];
        m[1] += term1;
    }

    // Write results to the buffer
    buffer[threadIdx*6] = n;
    buffer[threadIdx*6 + 1] = m[0];
    buffer[threadIdx*6 + 2] = m[1];
    buffer[threadIdx*6 + 3] = m[2];
    buffer[threadIdx*6 + 4] = m[3];
    buffer[threadIdx*6 + 5] = integerOnly;
}

)CLC";
