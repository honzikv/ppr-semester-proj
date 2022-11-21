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

	int4 n = {0};
	int4 isIntegerDistribution = _mm256_set1_epi64x(UINT64_MAX);

public:
	void pushAll(const std::vector<double>& data) {

		// AVX2 uses 256-bit registers which can hold 4 doubles
		// so we need to process 4 doubles at a time
		// Rest is truncated - this is basically negligible
		for (auto i = 0ULL; i < data.size() / 4; i += 4) {
			// Convert to __m256d
			const auto x = _mm256_loadu_pd(&data[i]);

			// Check whether values are valid
			auto valid = VectorizationUtils::valuesValid(x);

			// If the values are valid check if they are integers
			// To update isIntegerDistribution we have two boolean variables: valid and isInteger
			// We must satisfy that if valid is false we always return true and if valid is true we return isInteger
			// Therefore the formula is: !valid || isInteger
			const auto isInteger = VectorizationUtils::valuesInteger(x);
			const auto validNegation = _mm256_xor_si256(valid, _mm256_set1_epi64x(UINT64_MAX));
			const auto isIntegerDistributionUpdate = _mm256_or_si256(validNegation, isInteger);

			// Finally isIntegerDistribution = isIntegerDistribution && !valid || isInteger
			isIntegerDistribution = _mm256_and_si256(isIntegerDistribution, isIntegerDistributionUpdate);

			// Now valid contains 1s where the values are valid and 0s where they are not
			// We can map this to 0x1 for 1s and keep 0x00 for 0s
			const auto validMask = _mm256_cvtepi64_pd(_mm256_and_si256(valid, _mm256_set1_epi64x(0x1)));

			// Compute update values as normal
			const auto n1 = _mm256_cvtepi64_pd(n);
			const auto delta = _mm256_sub_pd(x, m1); // delta = x - m1
			n = _mm256_add_epi64(n, valid); // n = n + 1
			const auto deltaN = _mm256_div_pd(delta, _mm256_cvtepi64_pd(n)); // deltaN = delta / n
			const auto deltaSquared = _mm256_mul_pd(delta, delta); // deltaSquared = delta * delta
			const auto term1 = _mm256_mul_pd(n1, _mm256_mul_pd(delta, deltaN)); // term1 = delta * deltaN

			// Use valid to mask the update values
			m1 = _mm256_add_pd(m1, _mm256_mul_pd(deltaN, validMask)); // m1 = m1 + deltaN
			// m2 = m2 + deltaSquared * (n - 1) - term1
			m2 = _mm256_add_pd(m2, _mm256_mul_pd(
				                   _mm256_sub_pd(_mm256_mul_pd(deltaSquared, _mm256_sub_pd(n1, validMask)), term1),
				                   validMask));
			// m3 = m3 + term1 * deltaN * (n - 2) - 3* deltaN * m2
			m3 = _mm256_add_pd(m3, _mm256_mul_pd(
				                   _mm256_sub_pd(
					                   _mm256_mul_pd(_mm256_mul_pd(term1, deltaN),
					                                 _mm256_sub_pd(n1, _mm256_set1_pd(2))),
					                   _mm256_mul_pd(_mm256_set1_pd(3), _mm256_mul_pd(deltaN, m2))), validMask));
			// m4 = term1
			m4 = _mm256_add_pd(m4, _mm256_mul_pd(term1, validMask));
		}
	}

	auto operator+(const RunningStatsAvx2& other) {
		auto result = RunningStatsAvx2();

		const auto delta = _mm256_sub_pd(other.m1, m1); // delta = other.m1 - m1
		const auto delta2 = _mm256_mul_pd(delta, delta); // delta2 = delta * delta
		const auto delta3 = _mm256_mul_pd(delta2, delta); // delta3 = delta2 * delta
		const auto delta4 = _mm256_mul_pd(delta2, delta2); // delta4 = delta2 * delta2
		// mean = m1 + delta / (n + other.n)
		const auto mean = _mm256_add_pd(m1, _mm256_div_pd(delta, _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n))));

		result.m1 = mean;
		// result.m2 = m2 + other.m2 + delta2 * n * other.n / result.n;
		result.m2 = _mm256_add_pd(m2, _mm256_add_pd(other.m2, _mm256_div_pd(
			                                            _mm256_mul_pd(delta2, _mm256_mul_pd(_mm256_cvtepi64_pd(n),
				                                                          _mm256_cvtepi64_pd(other.n))),
			                                            _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n)))));
		// result.m3 = m3 + other.m3 + delta3 * n * other.n * (n - other.n) / (result.n * result.n) +
		// 3.0 * delta * (n * other.m2 - other.n * m2) / result.n;
		result.m3 = _mm256_add_pd(m3, _mm256_add_pd(other.m3, _mm256_div_pd(
			                                            _mm256_add_pd(
				                                            _mm256_mul_pd(
					                                            delta3, _mm256_mul_pd(
						                                            _mm256_mul_pd(_mm256_cvtepi64_pd(n),
							                                            _mm256_cvtepi64_pd(other.n)),
						                                            _mm256_sub_pd(
							                                            _mm256_cvtepi64_pd(n),
							                                            _mm256_cvtepi64_pd(other.n)))),
				                                            _mm256_mul_pd(_mm256_set1_pd(3.0), _mm256_mul_pd(
					                                                          delta, _mm256_div_pd(
						                                                          _mm256_sub_pd(
							                                                          _mm256_mul_pd(
								                                                          _mm256_cvtepi64_pd(n),
								                                                          other.m2),
							                                                          _mm256_mul_pd(
								                                                          _mm256_cvtepi64_pd(other.n),
								                                                          m2)),
						                                                          _mm256_cvtepi64_pd(
							                                                          _mm256_add_epi64(n, other.n)))))),
			                                            _mm256_mul_pd(
				                                            _mm256_mul_pd(_mm256_mul_pd(delta2, _mm256_cvtepi64_pd(n)),
				                                                          _mm256_cvtepi64_pd(other.n)),
				                                            _mm256_div_pd(
					                                            _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n)),
					                                            _mm256_mul_pd(
						                                            _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n)),
						                                            _mm256_cvtepi64_pd(
							                                            _mm256_add_epi64(n, other.n))))))));

		// result.m4 = m4 + other.m4 + delta4 * n * other.n * (n * n - n * other.n + other.n * other.n) /
		// (result.n * result.n * result.n) +
		//	6.0 * delta2 * (n * n * other.m2 + other.n * other.n * m2) / (result.n * result.n) +
		//	4.0 * delta * (n * other.m3 - other.n * m3) / result.n;

		// m4 is too long so we split it into
		// m4a = m4 + other.m4 + delta4 * n * other.n * (n * n - n * other.n + other.n * other.n) / (result.n * result.n * result.n)
		// m4b = 6.0 * delta2 * (n * n * other.m2 + other.n * other.n * m2) / (result.n * result.n) + 4.0 * delta * (n * other.m3 - other.n * m3) / result.n
		// m4 = m4a + m4b

		const auto m4a = _mm256_add_pd(m4, _mm256_add_pd(other.m4, _mm256_div_pd(
			                                                 _mm256_mul_pd(
				                                                 delta4, _mm256_mul_pd(
					                                                 _mm256_mul_pd(_mm256_cvtepi64_pd(n),
						                                                 _mm256_cvtepi64_pd(other.n)),
					                                                 _mm256_add_pd(
						                                                 _mm256_sub_pd(
							                                                 _mm256_mul_pd(
								                                                 _mm256_cvtepi64_pd(n),
								                                                 _mm256_cvtepi64_pd(n)),
							                                                 _mm256_mul_pd(
								                                                 _mm256_cvtepi64_pd(n),
								                                                 _mm256_cvtepi64_pd(other.n))),
						                                                 _mm256_mul_pd(
							                                                 _mm256_cvtepi64_pd(other.n),
							                                                 _mm256_cvtepi64_pd(other.n))))),
			                                                 _mm256_mul_pd(
				                                                 _mm256_mul_pd(
					                                                 _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n)),
					                                                 _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n))),
				                                                 _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n))))));
		const auto m4b = _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(6.0), _mm256_div_pd(
			                                             _mm256_add_pd(
				                                             _mm256_mul_pd(
					                                             _mm256_mul_pd(
						                                             _mm256_cvtepi64_pd(n), _mm256_cvtepi64_pd(n)),
					                                             other.m2),
				                                             _mm256_mul_pd(
					                                             _mm256_mul_pd(
						                                             _mm256_cvtepi64_pd(other.n),
						                                             _mm256_cvtepi64_pd(other.n)),
					                                             m2)),
			                                             _mm256_mul_pd(
				                                             _mm256_mul_pd(
					                                             _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n)),
					                                             _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n))),
				                                             _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n))))),
		                               _mm256_div_pd(
			                               _mm256_mul_pd(_mm256_set1_pd(4.0), _mm256_sub_pd(
				                                             _mm256_mul_pd(_mm256_cvtepi64_pd(n), other.m3),
				                                             _mm256_mul_pd(_mm256_cvtepi64_pd(other.n), m3))),
			                               _mm256_cvtepi64_pd(_mm256_add_epi64(n, other.n))));

		result.m4 = _mm256_add_pd(m4a, m4b);

		// Vector AND isIntegerDistribution
		result.isIntegerDistribution = _mm256_and_si256(isIntegerDistribution, other.isIntegerDistribution);

		// return the result
		return result;
	}
};
