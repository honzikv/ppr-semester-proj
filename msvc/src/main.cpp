#include <array>

#include "Arghandling.h"
#include "DistributionClassification.h"
#include "VectorizationUtils.h"
#include "JobScheduler.h"
#include <cmath>

using double4 = __m256d;

class RunningStatsAvx2 {
	std::array<long long, 4> n = { 0, 0, 0, 0 }; // Number of items processed
	std::array<double, 4> m1 = { .0, .0, .0, .0 };
	std::array<double, 4> m2 = { .0, .0, .0, .0 };
	std::array<double, 4> m3 = { .0, .0, .0, .0 };
	std::array<double, 4> m4 = { .0, .0, .0, .0 };

	std::array<int, 4> isInt = { 1, 1, 1, 1 };

public:
	void processArray(const std::vector<double>& data) {
		// AVX2 uses 256-bit registers which can hold 4 doubles
		// so we need to process 4 doubles at a time
		// Rest is truncated - this is basically negligible
		for (auto i = 0; i < floor(data.size() / 4) * 4; i += 1) {
			const auto idx = i % 4;
			const auto n1 = n[idx];
			n[idx] += 1;

			const auto nVal = n[idx];
			const auto delta = data[i] - m1[idx];
			const auto deltaN = delta / n[idx];
			const auto deltaNSquared = deltaN * deltaN;
			const auto term1 = delta * deltaN * n1;
			//
			// m1[idx] += deltaN;
			// m4[idx] += term1 * deltaNSquared * (nVal * nVal - 3 * nVal + 3) + 6 * deltaNSquared * m2[idx] - 4 * deltaN * m3[idx];
			// m3[idx] += term1 * deltaN * (nVal - 2) - 3 * deltaN * m2[idx];
			// m2[idx] += term1;
		}
	}
};



#define true 1  // From hell
#define false 0
#define EXPONENT_MASK 0x7fffffffffffffffULL
#define MANTISSA_MASK 0x000fffffffffffffULL

// typedef ulong uint64_t;
// typedef int int16_t;

inline bool valueNormalOrZero(double x) {
	int64_t bits = *(int64_t*)&x;
	int64_t exponent = (bits & EXPONENT_MASK) >> 52;

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


int main(int argc, char** argv) {
	// auto args = parseArguments(argc, argv);
	// auto processingInfo = validateArguments(args);
	// auto jobScheduler = JobScheduler(processingInfo);
	//
	// auto result = jobScheduler.run();
	// classifyDistribution(result);

	// auto runningStats = RunningStatsAvx2{};
	//
	// auto data = std::vector<double>(100000);
	// runningStats.processArray(data);

	VectorizationUtils::testValuesInvalidFn();
	VectorizationUtils::TestValuesIntegerFn();

	return 0;

}
