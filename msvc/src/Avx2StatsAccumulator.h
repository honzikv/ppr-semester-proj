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

	int4 n = int4Set(0);
	int4 isIntegerDistribution = int4Set(UINT64_MAX);

public:
	void push(const double4 x) {
		// AVX2 uses 256-bit registers which can hold 4 doubles
		// so we need to process 4 doubles at a time
		// Rest is truncated - this is basically negligible

		// Check whether values are valid
		const auto valid = VectorizationUtils::valuesValid(x);

		// If the values are valid check if they are integers
		// To update isIntegerDistribution we have two boolean variables: valid and isInteger
		// We must satisfy that if valid is false we always return true and if valid is true we return isInteger
		// Therefore the formula is: !valid || isInteger
		const auto isInteger = VectorizationUtils::valuesInteger(x);
		const auto validNegation = int4Xor(valid, int4Set(UINT64_MAX));
		const auto isIntegerDistributionUpdate = int4Or(validNegation, isInteger);

		// Finally isIntegerDistribution = isIntegerDistribution && !valid || isInteger
		isIntegerDistribution = int4And(isIntegerDistribution, isIntegerDistributionUpdate);

		// Now valid contains 1s where the values are valid and 0s where they are not
		// We can map this to 0x1 for 1s and keep 0x00 for 0s
		const auto validMask = int4ToDouble4(int4And(valid, int4Set(0x1)));

		const auto n1 = int4ToDouble4(n); // n1 = n, we convert to double to avoid casting later
		n = int4Add(n, int4And(int4Set(1), valid)); // n += valid * 1

		const auto nDouble = int4ToDouble4(n); // nDouble = n, we convert to double to avoid casting later
		const auto delta = double4Sub(x, m1); // delta = x - m1
		const auto deltaN = double4Div(delta, nDouble); // deltaN = delta / n
		const auto deltaNSquared = double4Mul(deltaN, deltaN); // deltaNSquared = deltaN * deltaN
		const auto term1 = double4Mul(delta, double4Mul(deltaN, n1)); // term1 = delta * deltaN * n1

		// m1 = m1 + validMask * deltaN
		m1 = double4Add(m1, double4Mul(deltaN, validMask));

		// m4 += (term1 * deltaNSquared) * (n * n - 3 * n + 3) + 6 * deltaNSquared * m2 - 4 * deltaN * m3
		// m4a1 = term1 * deltaNSquared
		// m4a2 = (((n * n) - (3 * n)) + 3)
		// m4a = m4a1 * m4a2
		const auto m4a1 = double4Mul(term1, deltaNSquared);
		// const auto m4a2 = double4Add(double4Sub(double4Mul(nDouble, nDouble), double4Mul(double4Set(3), nDouble)), double4Set(3));
		const auto m4a2 = double4Add(double4Set(3), double4Sub(double4Mul(nDouble, nDouble), double4Mul(double4Set(3), nDouble)));
		const auto m4a = double4Mul(m4a1, m4a2);

		// m4b1 = 6 * (deltaNSquared * m2)
		// m4b2 = 4 * (deltaN * m3)
		// m4b = m4b1 - m4b2
		const auto m4b1 = double4Mul(double4Set(6), double4Mul(deltaNSquared, m2));
		const auto m4b2 = double4Mul(double4Set(4), double4Mul(deltaN, m3));
		const auto m4b = double4Sub(m4b1, m4b2);

		// m4 = m4 + validMask * (m4a + m4b)
		m4 = double4Add(m4, double4Mul(validMask, double4Add(m4a, m4b)));

		// m3 += term1 * deltaN * (n - 2) - 3 * deltaN * m2
		// m3a = term1 * (deltaN * (n - 2))
		// m3b = 3 * (deltaN * m4)
		// m3 = m3 + (validMask * (m3a - m3b))
		const auto m3a = double4Mul(term1, double4Mul(deltaN, double4Sub(nDouble, double4Set(2))));
		const auto m3b = double4Mul(double4Set(3), double4Mul(deltaN, m2));
		m3 = double4Add(m3, double4Mul(validMask, double4Sub(m3a, m3b)));

		// m2 = m2 + (validMask * term1)
		m2 = double4Add(m2, double4Mul(validMask, term1));
	}

	auto operator+(const Avx2StatsAccumulator& other) const {
		auto result = Avx2StatsAccumulator();

		// result.n = n + other.n
		result.n = int4Add(n, other.n);

		const auto delta = double4Sub(other.m1, m1); // delta = other.m1 - m1
		const auto delta2 = double4Mul(delta, delta); // delta2 = delta * delta
		const auto delta3 = double4Mul(delta2, delta); // delta3 = delta2 * delta
		const auto delta4 = double4Mul(delta2, delta2); // delta4 = delta2 * delta2

		const auto nDouble = int4ToDouble4(n); // nDouble = n, we convert to double to avoid casting later
		const auto otherNDouble = int4ToDouble4(other.n);
		// otherNDouble = other.n, we convert to double to avoid casting later

		// mean = (n * m1 + other.n * other.m1) / (n + other.n)
		// meanA = ((n * m1) + (other.n * other.m1))
		// meanB = (n + other.n)
		// mean = meanA / meanB
		const auto meanA = double4Add(double4Mul(nDouble, m1), double4Mul(otherNDouble, other.m1));
		const auto meanB = double4Add(nDouble, otherNDouble);
		const auto mean = double4Div(meanA, meanB);
		result.m1 = mean;

		// result.m2 = m2 + other.m2 + delta2 * n * other.n / result.n

		// m2`a = m2 + other.m2
		const auto m2a = double4Add(m2, other.m2);

		// m2`b = (delta2 * n) * (other.n / result.n)
		const auto m2b = double4Mul(double4Mul(delta2, nDouble), double4Div(otherNDouble, nDouble));

		// result.m2 = m2 + m2a + m2b
		result.m2 = double4Add(m2, double4Add(m2a, m2b));

		// result.m3 = m3 + other.m3 + delta3 * n * other.n * (n - other.n) / (result.n * result.n)
		// + 3.0 * delta * (n * other.m2 - other.n * m2) / result.n

		const auto resultNDouble = int4ToDouble4(result.n);
		 
		// m3`a = m3 + other.m3
		const auto m3a = double4Add(m3, other.m3);

		// m3`b = delta3 * (n * other.n)
		const auto m3b = double4Mul(delta3, double4Mul(nDouble, otherNDouble));

		// m3`c = (n - other.n) / (result.n * result.n)
		const auto m3c = double4Div(double4Sub(nDouble, otherNDouble), double4Mul(resultNDouble, resultNDouble));

		// m3`d = 3.0 * delta
		const auto m3d = double4Mul(double4Set(3.0), delta);

		// m3`e = ((n * other.m2) - (other.n * m2)) / result.n
		const auto m3e = double4Div(double4Sub(double4Mul(nDouble, other.m2), double4Mul(otherNDouble, m2)),
		                            resultNDouble);

		// m3` = m3`a + ((m3`b * m3`c) + (m3`d * m3`e))
		result.m3 = double4Add(m3, double4Add(m3a, double4Add(double4Mul(m3b, m3c), double4Mul(m3d, m3e))));

		// result.m4 = m4 + other.m4 +
		// delta4 * n * other.n *
		// (n * n - n * other.n + other.n * other.n) / (result.n * result.n * result.n) +
		// 6.0 * delta2 *
		// (n * n * other.m2 + other.n * other.n * m2) / (result.n * result.n) +
		// 4.0 * delta * (n * other.m3 - other.n * m3) / result.n

		// m4`a = m4 + other.m4
		const auto m4a = double4Add(m4, other.m4);

		// m4`b1 = delta4 * (n * other.n)
		// m4`b2 = (((n * n) - (n * other.n)) + (other.n * other.n))
		const auto m4b1 = double4Mul(delta4, double4Mul(nDouble, otherNDouble));
		const auto m4b2 = double4Add(double4Sub(double4Mul(nDouble, nDouble), double4Mul(nDouble, otherNDouble)),
		                             double4Mul(otherNDouble, otherNDouble));

		// m4`c = (result.n * (result.n * result.n))
		const auto m4c = double4Mul(resultNDouble, double4Mul(resultNDouble, resultNDouble));

		// m4`d1 = 6.0 * delta2
		// m4`d2 = (((n * n) * other.m2) + ((other.n * other.n) * m2)))
		const auto m4d1 = double4Mul(double4Set(6.0), delta2);
		const auto m4d2 = double4Add(double4Mul(double4Mul(nDouble, nDouble), other.m2),
		                             double4Mul(double4Mul(otherNDouble, otherNDouble), m2));

		// m4`e = (result.n * result.n)
		const auto m4e = double4Mul(resultNDouble, resultNDouble);

		// m4`f1 = 4.0 * delta
		// m4`f2 = ((n * other.m3) - (other.n * m3)) / result.n
		// m4`f = m4`f1 * m4`f2
		const auto m4f1 = double4Mul(double4Set(4.0), delta);
		const auto m4f2 = double4Div(double4Sub(double4Mul(nDouble, other.m3), double4Mul(otherNDouble, m3)),
		                             resultNDouble);
		const auto m4f = double4Mul(m4f1, m4f2);

		// m4` = (m4`a + ((m4`b1 * (m4`b2 / m4`c))) + ((m4`d1 * (m4`d2 / m4`e)) + m4`f))
		result.m4 = double4Add(m4, double4Add(m4a, double4Add(double4Mul(m4b1, double4Div(m4b2, m4c)),
		                                                      double4Add(double4Mul(m4d1, double4Div(m4d2, m4e)),
		                                                                 m4f))));

		result.isIntegerDistribution = int4And(isIntegerDistribution, other.isIntegerDistribution);

		// return the result
		return result;
	}

	auto& operator+=(const Avx2StatsAccumulator& rhs) {
		const auto combined = *this + rhs;
		*this = combined;
		return *this;
	}

	[[nodiscard]] auto asScalar() const {
		auto results = std::vector<StatsAccumulator>(4);
		
		for (auto i = 0; i < 4; i += 1) {
			// Simply map each vector element to one StatsAccumulator object
			results[i].n = static_cast<size_t>(n.m256i_i64[i]);
			results[i].m1 = m1.m256d_f64[i];
			results[i].m2 = m2.m256d_f64[i];
			results[i].m3 = m3.m256d_f64[i];
			results[i].m4 = m4.m256d_f64[i];
			results[i].isIntegerDistribution = static_cast<bool>(isIntegerDistribution.m256i_i64[i]);
		}
		
		auto result = results[0];
		for (auto i = 1; i < 4; i += 1) {
			result += results[i];
		}

		return result;
	}
};
