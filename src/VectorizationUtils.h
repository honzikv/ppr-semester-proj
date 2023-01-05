#pragma once
#include <immintrin.h>
#include <iostream>
#include <array>


// Since this is not natively supported by AVX2
// Adapted from https://stackoverflow.com/questions/41144668/how-to-efficiently-perform-double-int64-conversions-with-sse-avx
inline auto convertInt4ToDouble4(const __m256i x) {
	const auto magicILo = _mm256_set1_epi64x(0x4330000000000000); /* 2^52               encoded as floating-point  */
	const auto magicIHi32 = _mm256_set1_epi64x(0x4530000080000000); /* 2^84 + 2^63        encoded as floating-point  */
	const auto magicIAll = _mm256_set1_epi64x(0x4530000080100000); /* 2^84 + 2^63 + 2^52 encoded as floating-point  */
	const auto magicDAll = _mm256_castsi256_pd(magicIAll);

	const auto vLo = _mm256_blend_epi32(magicILo, x, 0b01010101);
	/* Blend the 32 lowest significant bits of x with magic_int_lo                                                   */
	auto vHi = _mm256_srli_epi64(x, 32);
	/* Extract the 32 most significant bits of x                                                                     */
	vHi = _mm256_xor_si256(vHi, magicIHi32);
	/* Flip the msb of v_hi and blend with 0x45300000                                                                */
	const auto vHiDbl = _mm256_sub_pd(_mm256_castsi256_pd(vHi), magicDAll);
	/* Compute in double precision:                                                                                  */
	const auto result = _mm256_add_pd(vHiDbl, _mm256_castsi256_pd(vLo));
	/* (v_hi - magic_d_all) + v_lo  Do not assume associativity of floating point addition !!                        */
	return result;
}


const auto EXPONENT_MASK = _mm256_set1_epi64x(0x7fffffffffffffffULL);
const auto MANTISSA_MASK = _mm256_set1_epi64x(0x000fffffffffffffULL);

namespace VectorizationUtils {

	/**
	 * \brief A customized implementation of std::fpclassify
	 *		  This function is used to check if a double is normal or zero
	 *
	 * \param x vector to check
	 *
	 * \return "Boolean"-like vector - either containing 111..111 or 000..000 where 111..111 is true and 000..000 is false
	 */
	inline auto valuesValid(const __m256d& x) {
		const auto bits = _mm256_castpd_si256(x); // convert x to integer vector
		auto exponent = _mm256_and_si256(bits, EXPONENT_MASK); // extract exponent
		exponent = _mm256_srli_epi64(exponent, 52); // shift by 52 bits to get mantissa

		// if exponent == 0
		const auto expEqualsZero = _mm256_cmpeq_epi64(exponent, _mm256_setzero_si256());

		// AND bits & MANTISSA_MASK > 0
		auto bitsAndMantissa = _mm256_and_si256(bits, MANTISSA_MASK);
		bitsAndMantissa = _mm256_cmpgt_epi64(bitsAndMantissa, _mm256_setzero_si256());

		// If exponent == 0 && bits & MANTISSA_MASK set to true
		const auto invalid1 = _mm256_and_si256(expEqualsZero, bitsAndMantissa);

		// Similarly if exponent == 0x7ff it is invalid as well
		const auto invalid2 = _mm256_cmpeq_epi64(exponent, _mm256_set1_epi64x(0x7ff));

		// Combine both with OR operation
		// This will return for each element in the vector if they are invalid
		const auto invalid = _mm256_or_si256(invalid1, invalid2);

		// Negate - i.e. XOR with 0xFFFFFFFFFFFFFFFF == int64_t(-1) == UINT64_MAX
		return _mm256_xor_si256(invalid, _mm256_set1_epi64x(UINT64_MAX));
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
	 * \brief Masks given double4 vector with given "boolean" vector and returns the result as double4 vector
	 * \param value value to be masked - this value is set to 0 if the mask is false, otherwise it is left unchanged
	 * \param mask mask containing either 1s or 0s depending on the value
	 * \return masked variant of the vector, 0 where the mask is false, original values where the mask is true
	 */
	inline auto maskDouble4(const __m256d value, const __m256i mask) {
		return _mm256_castsi256_pd(_mm256_and_si256(_mm256_castpd_si256(value), mask));
	}

	/**
	 * \brief A simple printout to console that tests some values
	 */
	inline void testValuesInvalidFn() {
		// Test with 0, 1, INF, and NAN
		const auto test1 = _mm256_set_pd(0.0, 1.0, NAN, INFINITY);

		// Test with denormal 1.0e-308, denormal DBL_MIN/2, -4.5, and 0.0
		const auto test2 = _mm256_set_pd(-0.0, DBL_MIN / 2, 1.182617175, 1.0e-308);

		// Convert to boolean array
		const auto invalid1 = valuesValid(test1);
		const auto booleanInvalid2 = std::array<bool, 4>{
			static_cast<bool>(invalid1.m256i_i64[0]),
			static_cast<bool>(invalid1.m256i_i64[1]),
			static_cast<bool>(invalid1.m256i_i64[2]),
			static_cast<bool>(invalid1.m256i_i64[3])
		};

		const auto invalid2 = valuesValid(test2);
		const auto booleanInvalid1 = std::array<bool, 4>{
			static_cast<bool>(invalid2.m256i_i64[0]),
			static_cast<bool>(invalid2.m256i_i64[1]),
			static_cast<bool>(invalid2.m256i_i64[2]),
			static_cast<bool>(invalid2.m256i_i64[3])
		};

		// Print results
		std::cout << "Test 1: " << std::endl;
		std::cout << "Is 0 valid: " << booleanInvalid2[3] << std::endl; // we index from 3 due to little endianness
		std::cout << "Is 1 valid: " << booleanInvalid2[2] << std::endl;
		std::cout << "Is NAN valid: " << booleanInvalid2[1] << std::endl;
		std::cout << "Is INF valid: " << booleanInvalid2[0] << std::endl;

		std::cout << "Test 2: " << std::endl;
		std::cout << "Is -0 valid: " << booleanInvalid1[3] << std::endl;
		std::cout << "Is DBL_MIN/2 valid: " << booleanInvalid1[2] << std::endl;
		std::cout << "Is -4.5 valid: " << booleanInvalid1[1] << std::endl;
		std::cout << "Is 1.0e-308 valid: " << booleanInvalid1[0] << std::endl;
	}

	inline void testValuesIntegerFn() {
		// Test with 0, 1.0, 1.5, 999.999
		const auto test = _mm256_set_pd(0.0, 1.0, 1.5, 999.999);

		// Results
		const auto results = valuesInteger(test);
		const auto booleanResults = std::array<bool, 4>{
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

	inline void testValidMasking() {
		// Test for several values

		const __m256d inputValues = {INFINITY, NAN, 1.0, 0.0};
		const __m256d myValues = {5, 5, 4, 5};

		const auto validMask = valuesValid(inputValues);
		const auto maskedValues = maskDouble4(inputValues, validMask);

		// Add to myValues
		const auto result = _mm256_add_pd(maskedValues, myValues);

		// Print the values - they should all be 5.0
		for (int i = 0; i < 4; i++) {
			std::cout << "Result " << i << ": " << result.m256d_f64[i] << std::endl;
		}

	}
}
