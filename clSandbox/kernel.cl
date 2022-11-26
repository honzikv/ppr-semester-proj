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
    }
    
    // Write results to the buffer
    buffer[threadIdx * 6] = n;
    buffer[threadIdx * 6 + 1] = m1;
    buffer[threadIdx * 6 + 2] = m2;
    buffer[threadIdx * 6 + 3] = m3;
    buffer[threadIdx * 6 + 4] = m4;
    buffer[threadIdx * 6 + 5] = integerOnly;
}


__kernel void computeStatsFUNCTIONAL(__global double* buffer, int numElements) {
    size_t threadIdx = get_global_id(0);

    // Emulate a StatsAccumulator object
    int n = 0;
    bool integerOnly = true;
    double m1 = 0.0, m2 = 0.0, m3 = 0.0, m4 = 0.0;
    double intPart = 0.0;

    // Process numElements elements
    for (int i = 0; i < numElements; i += 1) {
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
