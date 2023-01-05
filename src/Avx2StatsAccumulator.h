#pragma once
#include <immintrin.h>
#include <vector>

#include "StatsAccumulator.h"
#include "VectorizationUtils.h"

/**
 * \brief Implementation of StatsAccumulator with AVX2 manual vectorization
 */
class Avx2StatsAccumulator {
	__m256d m1 = _mm256_set1_pd(0), m2 = _mm256_set1_pd(0), m3 = _mm256_set1_pd(0), m4 = _mm256_set1_pd(0);
	__m256d minVal = _mm256_set1_pd(std::numeric_limits<double>::infinity());
	__m256i n = _mm256_set1_epi64x(0);
	__m256i isIntegerDistribution = _mm256_set1_epi64x(UINT64_MAX);

public:
	/**
	 * \brief Pushes vector x to the accumulator, this vector can be dirty - i.e. contain invalid or NaN values
	 * \param x vector of doubles
	 */
	void pushWithFiltering(const __m256d x);

	/**
	 * \brief Pushes vector x to the accumulator, this vector must be contain only valid values that are FP_NORMAL or FP_ZERO
	 * \param x vector of doubles
	 */
	void push(const __m256d x);

	// Both + and += only work if the accumulators are not empty
	Avx2StatsAccumulator operator+(const Avx2StatsAccumulator& other) const;
	Avx2StatsAccumulator& operator+=(const Avx2StatsAccumulator& rhs);

	/**
	 * \brief Unpacks vectorized version, producing vector of four StatsAccumulators
	 * \return vector of four StatsAccumulator items
	 */
	[[nodiscard]] std::vector<StatsAccumulator> asVectorOfScalars() const;

	/**
	 * \brief Combines items in the vector into a single accumulator via left to right addition
	 * \return combined StatsAccumulator
	 */
	[[nodiscard]] StatsAccumulator asScalar() const;


};
