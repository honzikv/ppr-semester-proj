#pragma once

constexpr auto CL_PROGRAM = R"CLC(
#define EXPONENT_MASK 0x7fffffffffffffffULL
#define MANTISSA_MASK 0x000fffffffffffffULL
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
typedef ulong uint64_t;
typedef uint uint32_t;

// Modification of std::fpclassify
// Returns true if the number is FP_NORMAL or FP_ZERO
// Adapted from https://opensource.apple.com/source/Libm/Libm-315/Source/ARM/fpclassify.c.auto.html
inline bool valueNormalOrZero(double x) {
    union{ double d; uint64_t u;} u = {x};
    uint32_t exponent = (uint32_t) ( (u.u & 0x7fffffffffffffffULL) >> 52 );

    if (exponent == 0) {
        if (u.u & MANTISSA_MASK) {
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

    // FP_NORMAL
    return true;
}

__kernel void computeStats(__global double* data, __global double* stats, uint64_t numElements) {
    size_t threadIdx = get_global_id(0);
    uint64_t threadOffset = threadIdx * 7;

    // Emulate a RunningStats object
    // Load data from stats array
    uint64_t n = stats[threadOffset];
    double m1 = stats[threadOffset + 1], m2 = stats[threadOffset + 2], m3 = stats[threadOffset + 3], m4 = stats[threadOffset + 4];
    bool integerOnly = (bool) stats[threadOffset + 5];
    double min = stats[threadOffset + 6];
    double intPart = 0.0;

    // Process numElements elements
    for (uint64_t i = 0; i < numElements; i += 1) {
        double x = data[threadIdx*numElements+i];
        if (!valueNormalOrZero(x)) {
            // Skip if x is NaN, infinity or denormal
            continue;
        }

        double n1 = n;

        n += 1;
        integerOnly = integerOnly && modf(x, &intPart) == 0.0;

        double delta = x - m1;
        double deltaN = delta / n;
        double deltaNSquared = deltaN * deltaN;
        double term1 = delta * deltaN * n1;
        m1 += deltaN;
        m4 += term1 * deltaNSquared * (n * n - 3 * n + 3) + 6 * deltaNSquared * m2 - 4 * deltaN * m3;
        m3 += term1 * deltaN * (n - 2) - 3 * deltaN * m2;
        m2 += term1;
        min = fmin(min, x);
    }
    
    // Write results to the data
    stats[threadOffset] = n;
    stats[threadOffset + 1] = m1;
    stats[threadOffset + 2] = m2;
    stats[threadOffset + 3] = m3;
    stats[threadOffset + 4] = m4;
    stats[threadOffset + 5] = integerOnly;
    stats[threadOffset + 6] = min;
}
)CLC";
