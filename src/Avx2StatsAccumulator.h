#pragma once
#include <immintrin.h>
#include <vector>

#include "StatsAccumulator.h"
#include "VectorizationUtils.h"

/**
 * \brief Implementation of StatsAccumulator with AVX2 manual vectorization
 */
class Avx2StatsAccumulator {
	double4 m1 = double4Set(0), m2 = double4Set(0), m3 = double4Set(0), m4 = double4Set(0);
	double4 min = double4Set(std::numeric_limits<double>::infinity());
	int4 n = int4Set(0);
	int4 isIntegerDistribution = int4Set(UINT64_MAX);

public:
	/**
	 * \brief Pushes vector x to the accumulator, this vector can be dirty - i.e. contain invalid or NaN values
	 * \param x vector of doubles
	 */
	void pushWithFiltering(const double4 x);

	/**
	 * \brief Pushes vector x to the accumulator, this vector must be contain only valid values that are FP_NORMAL or FP_ZERO
	 * \param x vector of doubles
	 */
	void push(const double4 x);

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
