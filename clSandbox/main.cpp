#include <iostream>
#include "src/clTest.h"

double modf(double x) {
    union {double f; uint64_t i;} u = {x};
    uint64_t mask;
    int16_t exponent = (int16_t)(u.i>>52 & 0x7ff) - 0x3ff;

    if (exponent >= 52) {
        u.i &= 1ULL<<63;
        return u.f;
    }

    if (exponent < 0) {
        u.i &= 1ULL<<63;
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

int main() {

    testClKernel();

    return 0;
}
