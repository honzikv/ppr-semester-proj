#include "Avx2StatsAccumulator.h"

void Avx2StatsAccumulator::pushWithFiltering(const double4 x) {
	// AVX2 uses 256-bit registers which can hold 4 doubles
	// so we need to process 4 doubles at a time

	// Check whether values are valid
	const auto validMask = VectorizationUtils::valuesValid(x);

	// If the values are valid check if they are integers
	// To update isIntegerDistribution we have two boolean variables: valid and isInteger
	// We must satisfy that if valid is false we always return true and if valid is true we return isInteger
	// Boolean formula for this is: !valid || isInteger
	const auto isInteger = VectorizationUtils::valuesInteger(x);
	const auto validNegation = _mm256_xor_si256(validMask, _mm256_set1_epi64x(UINT64_MAX));
	const auto isIntegerDistributionUpdate = _mm256_or_si256(validNegation, isInteger);

	// Update isIntegerDistribution = isIntegerDistribution && !valid || isInteger
	isIntegerDistribution = _mm256_and_si256(isIntegerDistribution, isIntegerDistributionUpdate);

	// Update the minimum
	// If the value is valid we keep the value of xi, if the value is invalid we set the value of xi to INFINITY
	const auto minMask = VectorizationUtils::maskDouble4(_mm256_set1_pd(std::numeric_limits<double>::infinity()), validNegation);
	minVal = _mm256_min_pd(minVal, _mm256_add_pd(x, minMask));

	const auto n1 = convertInt4ToDouble4(n); // n1 = n, we convert to double to avoid casting later
	n = _mm256_add_epi64(n, _mm256_and_si256(_mm256_set1_epi64x(1), validMask)); // n += valid * 1

	const auto nDouble = convertInt4ToDouble4(n); // nDouble = n, we convert to double to avoid casting later
	const auto delta = _mm256_sub_pd(x, m1); // delta = x - m1
	const auto deltaN = _mm256_div_pd(delta, nDouble); // deltaN = delta / n
	const auto deltaNSquared = _mm256_mul_pd(deltaN, deltaN); // deltaNSquared = deltaN * deltaN
	const auto term1 = _mm256_mul_pd(delta, _mm256_mul_pd(deltaN, n1)); // term1 = delta * deltaN * n1

	// m1 = m1 + validMask * deltaN
	m1 = _mm256_add_pd(m1, VectorizationUtils::maskDouble4(deltaN, validMask));

	// m4 += (term1 * deltaNSquared) * (n * n - 3 * n + 3) + 6 * deltaNSquared * m2 - 4 * deltaN * m3
	// m4a1 = term1 * deltaNSquared
	// m4a2 = (((n * n) - (3 * n)) + 3)
	// m4a = m4a1 * m4a2
	const auto m4a1 = _mm256_mul_pd(term1, deltaNSquared);
	// const auto m4a2 = _mm256_add_pd(_mm256_sub_pd(_mm256_mul_pd(nDouble, nDouble), _mm256_mul_pd(_mm256_set1_pd(3), nDouble)), _mm256_set1_pd(3));
	const auto m4a2 = _mm256_add_pd(
		_mm256_set1_pd(3), _mm256_sub_pd(_mm256_mul_pd(nDouble, nDouble), _mm256_mul_pd(_mm256_set1_pd(3), nDouble)));
	const auto m4a = _mm256_mul_pd(m4a1, m4a2);

	// m4b1 = 6 * (deltaNSquared * m2)
	// m4b2 = 4 * (deltaN * m3)
	// m4b = m4b1 - m4b2
	const auto m4b1 = _mm256_mul_pd(_mm256_set1_pd(6), _mm256_mul_pd(deltaNSquared, m2));
	const auto m4b2 = _mm256_mul_pd(_mm256_set1_pd(4), _mm256_mul_pd(deltaN, m3));
	const auto m4b = _mm256_sub_pd(m4b1, m4b2);

	// m4 = m4 + validMask * (m4a + m4b)
	m4 = _mm256_add_pd(m4, VectorizationUtils::maskDouble4(_mm256_add_pd(m4a, m4b), validMask));

	// m3 += term1 * deltaN * (n - 2) - 3 * deltaN * m2
	// m3a = term1 * (deltaN * (n - 2))
	// m3b = 3 * (deltaN * m4)
	// m3 = m3 + (validMask * (m3a - m3b))
	const auto m3a = _mm256_mul_pd(term1, _mm256_mul_pd(deltaN, _mm256_sub_pd(nDouble, _mm256_set1_pd(2))));
	const auto m3b = _mm256_mul_pd(_mm256_set1_pd(3), _mm256_mul_pd(deltaN, m2));
	m3 = _mm256_add_pd(m3, VectorizationUtils::maskDouble4(_mm256_sub_pd(m3a, m3b), validMask));

	// m2 = m2 + (validMask * term1)
	m2 = _mm256_add_pd(m2, VectorizationUtils::maskDouble4(term1, validMask));
}

void Avx2StatsAccumulator::push(const double4 x) {
	isIntegerDistribution = _mm256_and_si256(isIntegerDistribution, VectorizationUtils::valuesInteger(x));

	const auto n1 = convertInt4ToDouble4(n); // n1 = n
	n = _mm256_add_epi64(n, _mm256_set1_epi64x(1)); // n += 1

	const auto nDouble = convertInt4ToDouble4(n); // nDouble = n
	const auto delta = _mm256_sub_pd(x, m1); // delta = x - m1
	const auto deltaN = _mm256_div_pd(delta, nDouble); // deltaN = delta / n
	const auto deltaNSquared = _mm256_mul_pd(deltaN, deltaN); // deltaNSquared = deltaN * deltaN
	const auto term1 = _mm256_mul_pd(delta, _mm256_mul_pd(deltaN, n1)); // term1 = delta * deltaN * n1

	m1 = _mm256_add_pd(m1, deltaN); // m1 += deltaN

	// m4 += (term1 * deltaNSquared) * (n * n - 3 * n + 3) + 6 * deltaNSquared * m2 - 4 * deltaN * m3

	// m4a1 = term1 * deltaNSquared
	const auto m4a1 = _mm256_mul_pd(term1, deltaNSquared);

	// m4a2 = ((n * n) - (3 * n) + 3)
	const auto m4a2 = _mm256_add_pd(
		_mm256_set1_pd(3), _mm256_sub_pd(_mm256_mul_pd(nDouble, nDouble), _mm256_mul_pd(_mm256_set1_pd(3), nDouble)));

	// m4b1 = 6 * deltaNSquared * m2
	const auto m4b1 = _mm256_mul_pd(_mm256_set1_pd(6), _mm256_mul_pd(deltaNSquared, m2));

	// m4b2 = 4 * (deltaN * m3)
	const auto m4b2 = _mm256_mul_pd(_mm256_set1_pd(4), _mm256_mul_pd(deltaN, m3));

	// m4a = m4a1 * m4a2
	const auto m4a = _mm256_mul_pd(m4a1, m4a2);

	// m4b = m4b1 - m4b2
	const auto m4b = _mm256_sub_pd(m4b1, m4b2);

	// m4 += m4a + m4b
	m4 = _mm256_add_pd(m4, _mm256_add_pd(m4a, m4b));

	// m3 += term1 * deltaN * (n - 2) - 3 * deltaN * m2

	// m3a = term1 * (deltaN * (n - 2))
	const auto m3a = _mm256_mul_pd(term1, _mm256_mul_pd(deltaN, _mm256_sub_pd(nDouble, _mm256_set1_pd(2))));

	// m3b = 3 * (deltaN * m2)
	const auto m3b = _mm256_mul_pd(_mm256_set1_pd(3), _mm256_mul_pd(deltaN, m2));

	// m3 += m3a - m3b
	m3 = _mm256_add_pd(m3, _mm256_sub_pd(m3a, m3b));

	// m2 += term1
	m2 = _mm256_add_pd(m2, term1);
}

Avx2StatsAccumulator& Avx2StatsAccumulator::operator+=(const Avx2StatsAccumulator& rhs) {
	const auto combined = *this + rhs;
	*this = combined;
	return *this;
}

std::vector<StatsAccumulator> Avx2StatsAccumulator::asVectorOfScalars() const {
	auto results = std::vector<StatsAccumulator>(4);
	for (auto i = 0; i < 4; i += 1) {
		// Simply map each vector element to one StatsAccumulator object
		results[i].n = static_cast<size_t>(n.m256i_i64[i]);
		results[i].m1 = m1.m256d_f64[i];
		results[i].m2 = m2.m256d_f64[i];
		results[i].m3 = m3.m256d_f64[i];
		results[i].m4 = m4.m256d_f64[i];
		results[i].isIntegerDistribution = static_cast<bool>(isIntegerDistribution.m256i_i64[i]);
		results[i].minVal = minVal.m256d_f64[i];
	}

	return results;
}

StatsAccumulator Avx2StatsAccumulator::asScalar() const {
	auto results = asVectorOfScalars();
	auto& result = results[0];
	for (auto i = 1; i < 4; i += 1) {
		result += results[i];
	}

	return result;
}

Avx2StatsAccumulator Avx2StatsAccumulator::operator+(const Avx2StatsAccumulator& other) const {
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	auto result = Avx2StatsAccumulator();

	// result.n = n + other.n
	result.n = _mm256_add_epi64(n, other.n);

	const auto delta = _mm256_sub_pd(other.m1, m1); // delta = other.m1 - m1
	const auto delta2 = _mm256_mul_pd(delta, delta); // delta2 = delta * delta
	const auto delta3 = _mm256_mul_pd(delta2, delta); // delta3 = delta2 * delta
	const auto delta4 = _mm256_mul_pd(delta2, delta2); // delta4 = delta2 * delta2

	const auto nDouble = convertInt4ToDouble4(n); // nDouble = n, we convert to double to avoid casting later
	const auto otherNDouble = convertInt4ToDouble4(other.n);
	// otherNDouble = other.n, we convert to double to avoid casting later

	// mean = (n * m1 + other.n * other.m1) / (n + other.n)
	// meanA = ((n * m1) + (other.n * other.m1))
	// meanB = (n + other.n)
	// mean = meanA / meanB
	const auto meanA = _mm256_add_pd(_mm256_mul_pd(nDouble, m1), _mm256_mul_pd(otherNDouble, other.m1));
	const auto meanB = _mm256_add_pd(nDouble, otherNDouble);
	const auto mean = _mm256_div_pd(meanA, meanB);
	result.m1 = mean;

	// result.m2 = m2 + other.m2 + delta2 * n * other.n / result.n

	// m2`a = m2 + other.m2
	const auto m2a = _mm256_add_pd(m2, other.m2);

	// m2`b = (delta2 * n) * (other.n / result.n)
	const auto m2b = _mm256_mul_pd(_mm256_mul_pd(delta2, nDouble), _mm256_div_pd(otherNDouble, nDouble));

	// result.m2 = m2 + m2a + m2b
	result.m2 = _mm256_add_pd(m2, _mm256_add_pd(m2a, m2b));

	// result.m3 = m3 + other.m3 + delta3 * n * other.n * (n - other.n) / (result.n * result.n)
	// + 3.0 * delta * (n * other.m2 - other.n * m2) / result.n

	const auto resultNDouble = convertInt4ToDouble4(result.n);

	// m3`a = m3 + other.m3
	const auto m3a = _mm256_add_pd(m3, other.m3);

	// m3`b = delta3 * (n * other.n)
	const auto m3b = _mm256_mul_pd(delta3, _mm256_mul_pd(nDouble, otherNDouble));

	// m3`c = (n - other.n) / (result.n * result.n)
	const auto m3c = _mm256_div_pd(_mm256_sub_pd(nDouble, otherNDouble), _mm256_mul_pd(resultNDouble, resultNDouble));

	// m3`d = 3.0 * delta
	const auto m3d = _mm256_mul_pd(_mm256_set1_pd(3.0), delta);

	// m3`e = ((n * other.m2) - (other.n * m2)) / result.n
	const auto m3e = _mm256_div_pd(_mm256_sub_pd(_mm256_mul_pd(nDouble, other.m2), _mm256_mul_pd(otherNDouble, m2)),
	                            resultNDouble);

	// m3` = m3`a + ((m3`b * m3`c) + (m3`d * m3`e))
	result.m3 = _mm256_add_pd(m3, _mm256_add_pd(m3a, _mm256_add_pd(_mm256_mul_pd(m3b, m3c), _mm256_mul_pd(m3d, m3e))));

	// result.m4 = m4 + other.m4 +
	// delta4 * n * other.n *
	// (n * n - n * other.n + other.n * other.n) / (result.n * result.n * result.n) +
	// 6.0 * delta2 *
	// (n * n * other.m2 + other.n * other.n * m2) / (result.n * result.n) +
	// 4.0 * delta * (n * other.m3 - other.n * m3) / result.n

	// m4`a = m4 + other.m4
	const auto m4a = _mm256_add_pd(m4, other.m4);

	// m4`b1 = delta4 * (n * other.n)
	// m4`b2 = (((n * n) - (n * other.n)) + (other.n * other.n))
	const auto m4b1 = _mm256_mul_pd(delta4, _mm256_mul_pd(nDouble, otherNDouble));
	const auto m4b2 = _mm256_add_pd(_mm256_sub_pd(_mm256_mul_pd(nDouble, nDouble), _mm256_mul_pd(nDouble, otherNDouble)),
	                             _mm256_mul_pd(otherNDouble, otherNDouble));

	// m4`c = (result.n * (result.n * result.n))
	const auto m4c = _mm256_mul_pd(resultNDouble, _mm256_mul_pd(resultNDouble, resultNDouble));

	// m4`d1 = 6.0 * delta2
	// m4`d2 = (((n * n) * other.m2) + ((other.n * other.n) * m2)))
	const auto m4d1 = _mm256_mul_pd(_mm256_set1_pd(6.0), delta2);
	const auto m4d2 = _mm256_add_pd(_mm256_mul_pd(_mm256_mul_pd(nDouble, nDouble), other.m2),
	                             _mm256_mul_pd(_mm256_mul_pd(otherNDouble, otherNDouble), m2));

	// m4`e = (result.n * result.n)
	const auto m4e = _mm256_mul_pd(resultNDouble, resultNDouble);

	// m4`f1 = 4.0 * delta
	// m4`f2 = ((n * other.m3) - (other.n * m3)) / result.n
	// m4`f = m4`f1 * m4`f2
	const auto m4f1 = _mm256_mul_pd(_mm256_set1_pd(4.0), delta);
	const auto m4f2 = _mm256_div_pd(_mm256_sub_pd(_mm256_mul_pd(nDouble, other.m3), _mm256_mul_pd(otherNDouble, m3)),
	                             resultNDouble);
	const auto m4f = _mm256_mul_pd(m4f1, m4f2);

	// m4` = (m4`a + ((m4`b1 * (m4`b2 / m4`c))) + ((m4`d1 * (m4`d2 / m4`e)) + m4`f))
	result.m4 = _mm256_add_pd(m4, _mm256_add_pd(m4a, _mm256_add_pd(_mm256_mul_pd(m4b1, _mm256_div_pd(m4b2, m4c)),
	                                                      _mm256_add_pd(_mm256_mul_pd(m4d1, _mm256_div_pd(m4d2, m4e)),
	                                                                 m4f))));

	result.isIntegerDistribution = _mm256_and_si256(isIntegerDistribution, other.isIntegerDistribution);

	// return the result
	return result;
}
