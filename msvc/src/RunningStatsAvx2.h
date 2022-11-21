#pragma once
#include <immintrin.h>
#include <vector>

#include "VectorizationUtils.h"

// For better readability we type alias the AVX2 types
using double4 = __m256d;
using int4 = __m256i;

/**
 * \brief Implementation of RunningStats with AVX2 manual vectorization
 */
class RunningStatsAvx2 {
	double4 m1 = {0}, m2 = {0}, m3 = {0}, m4 = {0};

	int4 isIntegerDistribution;

public:
	void pushAll(const std::vector<double>& data) {

		// AVX2 uses 256-bit registers which can hold 4 doubles
		// so we need to process 4 doubles at a time
		// Rest is truncated - this is basically negligible
		for (auto i = 0ULL; i < data.size() / 4; i += 4) {
			// Convert to __m256d
			auto x = _mm256_loadu_pd(&data[i]);

			// Check whether values are valid
			// const auto invalid = VectorizationUtils::valuesInvalid();
		}
	}
};
