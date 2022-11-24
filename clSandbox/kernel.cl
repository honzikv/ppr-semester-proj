// #define true 1  // From hell
// #define false 0
#define EXPONENT_MASK 0x7fffffffffffffffULL
#define MANTISSA_MASK 0x000fffffffffffffULL

typedef ulong uint64_t;
typedef int int16_t;

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

// Custom implementation of modf
// We can omit some features and only return the fractional part
inline double modf(double x) {
    union {double f; uint64_t i;} u = {x};
    uint64_t mask;
    int16_t exponent = (int16_t) (u.i >> 52 & 0x7ff) - 0x3ff;

    if (exponent >= 52) {
        // This is not required since we check it in valueNormalOrZero
		// if (exponent == 0x400 && u.i<<12 != 0) /* nan */
		// 	return x;
		u.i &= 1ULL << 63;
		return u.f;
    }

    if (exponent < 0) {
		u.i &= 1ULL << 63;
		return x;
	}

    mask = -1ULL >> 12 >> exponent;
	if ((u.i & mask) == 0) {
		u.i &= 1ULL << 63;
		return u.f;
	}

	u.i &= ~mask;
	return x - u.f;
}

// An equivalent of RunningStats::push
inline void push(size_t* n, double* m, bool* integerOnly, double x) {
    if (!valueNormalOrZero(x)) {
        // Skip if x is NaN, infinity or denormal
        return;
    }

    n += 1;
    *integerOnly = integerOnly && modf(x) == 0;
    double delta = x - m[0];
    double deltaN = delta / *n;
    double deltaNSquared = deltaN * deltaN;
    double term1 = delta * deltaN * (*n - 1);

    m[0] += deltaN;
    m[3] += term1 * deltaNSquared * (*n * *n - 3 * *n + 3) + 6 * deltaNSquared * m[1] - 4 * deltaN * m[2];
    m[2] += term1 * deltaN * (*n - 2) - 3 * deltaNSquared * m[1];
    m[1] += term1;
}


__kernel void computeStats(__global double* buffer, uint64_t numElements) {
    size_t threadIdx = get_global_id(0);

    // Emulate a RunningStats object
    size_t n = 0;
    bool integerOnly = true;
    double m[4]; // m1, m2, m3, m4

    // Process numElements elements
    for (size_t i = 0; i < numElements; i += 1) {
        push(&n, m, &integerOnly, buffer[threadIdx*numElements + i]);
    }

    // Write results to the buffer
    buffer[threadIdx*6] = n;
    buffer[threadIdx*6 + 1] = m[0];
    buffer[threadIdx*6 + 2] = m[1];
    buffer[threadIdx*6 + 3] = m[2];
    buffer[threadIdx*6 + 4] = m[3];
    buffer[threadIdx*6 + 5] = integerOnly;
}

__kernel void processArray(__global double* inputArray, __global double* outputArray, int nItemsToProcess) {
    size_t threadIdx = get_global_id(0);
    for (size_t i = 0; i < nItemsToProcess; i += 1) {
        outputArray[i] = inputArray[i] * inputArray[i];
    }
}

__kernel void add(
        ulong n,
        global const double *a,
        global const double *b,
        global double *c
) {
    size_t i = get_global_id(0);
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}