#pragma once
#include <immintrin.h>

const auto EXPONENT_MASK = _mm256_set1_epi64x(0x7fffffffffffffffULL);
const auto MANTISSA_MASK = _mm256_set1_epi64x(0x000fffffffffffffULL);

/**
 * \brief A customized implementation of std::fpclassify
 *		  This function is used to check if a double is normal or zero
 * \param x 
 * \return integer vector that contains true values for non-valid doubles and false values for valid doubles
 */
inline auto valuesInvalid(const __m256d& x) {

	// Convert x to integer vector
	auto bits = _mm256_castpd_si256(x);

	// Extract exponent
	auto exponent = _mm256_and_si256(bits, EXPONENT_MASK);

	// Shift by 52 bits to get mantissa
	exponent = _mm256_srli_epi64(exponent, 52);

	// if exponent == 0
	const auto expEqualsZero = _mm256_cmpeq_epi64(exponent, _mm256_setzero_si256());

	// AND bits & MANTISSA_MASK
	const auto bitsAndMantissa = _mm256_and_si256(bits, MANTISSA_MASK);

	// If exponent == 0 && bits & MANTISSA_MASK set to true
	const auto invalid1 = _mm256_and_si256(expEqualsZero, bitsAndMantissa);

	// Similarly if exponent == 0x7ff it is invalid as well
	const auto invalid2 = _mm256_cmpeq_epi64(exponent, _mm256_set1_epi64x(0x7ff));

	// Combine both with OR operation
	// This will return for each element in the vector if they are invalid
	return _mm256_or_si256(invalid1, invalid2);
}

inline auto valuesInteger(const __m256d& x) {
	// Thankfully functionality for extracting fractional part is already implemented
	const auto integer = _mm256_round_pd(x, _MM_FROUND_TRUNC);
	const auto fraction = _mm256_sub_pd(x, integer);

	// Now we only need to check if fraction == 0.0
	const auto result = _mm256_cmp_pd(fraction, _mm256_setzero_pd(), _CMP_EQ_OQ);

	// Convert to "bool" array
	return _mm256_castpd_si256(result);
}

/**
 * \brief A simple printout to console that tests some values
 */
inline void testValuesInvalidFn() {
	// Test with 0, 1, INF and NAN
	auto test1 = _mm256_set_pd(0.0, 1.0, NAN, INFINITY);

	// Test with denormal 1.0e-308, 1000, -4.5 and 0.0
	auto test2 = _mm256_set_pd(-0.0, 1000.0, -4.5, 1.0e-308);

	// Convert to boolean array
	auto invalid1 = valuesInvalid(test1);
	auto booleanInvalid2 = std::array<bool, 4>{
		static_cast<bool>(invalid1.m256i_i64[0]),
		static_cast<bool>(invalid1.m256i_i64[1]),
		static_cast<bool>(invalid1.m256i_i64[2]),
		static_cast<bool>(invalid1.m256i_i64[3])
	};

	auto booleanInvalid1 = std::array<bool, 4>{
		static_cast<bool>(valuesInvalid(test2).m256i_i64[0]),
		static_cast<bool>(valuesInvalid(test2).m256i_i64[1]),
		static_cast<bool>(valuesInvalid(test2).m256i_i64[2]),
		static_cast<bool>(valuesInvalid(test2).m256i_i64[3])
	};

	// Print results
	// Gotta love intel endianness xd
	std::cout << "Test 1: " << std::endl;
	// For readability we negate it so we get "valid" values
	std::cout << "Is 0 valid: " << !booleanInvalid2[3] << std::endl;
	std::cout << "Is 1 valid: " << !booleanInvalid2[2] << std::endl;
	std::cout << "Is NAN valid: " << !booleanInvalid2[1] << std::endl;
	std::cout << "Is INF valid: " << !booleanInvalid2[0] << std::endl;

	std::cout << "Test 2: " << std::endl;
	std::cout << "Is -0 valid: " << !booleanInvalid1[3] << std::endl;
	std::cout << "Is 1000 valid: " << !booleanInvalid1[2] << std::endl;
	std::cout << "Is -4.5 valid: " << !booleanInvalid1[1] << std::endl;
	std::cout << "Is 1.0e-308 valid: " << !booleanInvalid1[0] << std::endl;
}

inline void TestValuesIntegerFn() {
	// Test with 0, 1.0, 1.5, 999.999
	auto test = _mm256_set_pd(0.0, 1.0, 1.5, 999.999);

	// Results
	auto results = valuesInteger(test);

	auto booleanResults = std::array<bool, 4>{
		static_cast<bool>(results.m256i_i64[0]),
		static_cast<bool>(results.m256i_i64[1]),
		static_cast<bool>(results.m256i_i64[2]),
		static_cast<bool>(results.m256i_i64[3])
	};

	// Print results:
	std::cout << "Is 0 integer: " << booleanResults[3] << std::endl;
	std::cout << "Is 1 integer: " << booleanResults[2] << std::endl;
	std::cout << "Is 1.5 integer: " << booleanResults[1] << std::endl;
	std::cout << "Is 999.999 integer: " << booleanResults[0] << std::endl;
}
