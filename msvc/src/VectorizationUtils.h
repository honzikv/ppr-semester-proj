#pragma once
#include <immintrin.h>
#include <iostream>

// To make the code somewhat readable we type alias frequently used intrinsics and types
using double4 = __m256d;
using int4 = __m256i;

#define double4Set  _mm256_set1_pd
#define int4Set  _mm256_set1_epi64x
#define double4ToInt4 _mm256_castpd_si256
#define double4Add _mm256_add_pd
#define double4Mul _mm256_mul_pd
#define double4Sub _mm256_sub_pd
#define double4Div _mm256_div_pd


inline auto int4ToDouble4(const int4 x) {
	__m256i magic_i_lo = _mm256_set1_epi64x(0x4330000000000000);                /* 2^52               encoded as floating-point  */
	__m256i magic_i_hi32 = _mm256_set1_epi64x(0x4530000080000000);                /* 2^84 + 2^63        encoded as floating-point  */
	__m256i magic_i_all = _mm256_set1_epi64x(0x4530000080100000);                /* 2^84 + 2^63 + 2^52 encoded as floating-point  */
	__m256d magic_d_all = _mm256_castsi256_pd(magic_i_all);

	__m256i v_lo = _mm256_blend_epi32(magic_i_lo, x, 0b01010101);         /* Blend the 32 lowest significant bits of x with magic_int_lo                                                   */
	__m256i v_hi = _mm256_srli_epi64(x, 32);                              /* Extract the 32 most significant bits of x                                                                     */
	v_hi = _mm256_xor_si256(v_hi, magic_i_hi32);                  /* Flip the msb of v_hi and blend with 0x45300000                                                                */
	__m256d v_hi_dbl = _mm256_sub_pd(_mm256_castsi256_pd(v_hi), magic_d_all); /* Compute in double precision:                                                                                  */
	__m256d result = _mm256_add_pd(v_hi_dbl, _mm256_castsi256_pd(v_lo));    /* (v_hi - magic_d_all) + v_lo  Do not assume associativity of floating point addition !!                        */
	return result;
}

#define int4Add _mm256_add_epi64

// "Boolean operations"
#define int4And _mm256_and_si256
#define int4CompareGreaterThan _mm256_cmpgt_epi64
#define int4CompareEquals _mm256_cmpeq_epi64
#define int4Or _mm256_or_si256
#define int4Xor _mm256_xor_si256

const auto EXPONENT_MASK = int4Set(0x7fffffffffffffffULL);
const auto MANTISSA_MASK = int4Set(0x000fffffffffffffULL);

namespace VectorizationUtils {

	/**
	 * \brief A customized implementation of std::fpclassify
	 *		  This function is used to check if a double is normal or zero
	 *
	 * \param x vector to check
	 *
	 * \return "Boolean"-like vector - either containing 111..111 or 000..000 where 111..111 is true and 000..000 is false
	 */
	inline auto valuesValid(const double4& x) {
		const auto bits = double4ToInt4(x); // convert x to integer vector
		auto exponent = int4And(bits, EXPONENT_MASK); // extract exponent
		exponent = _mm256_srli_epi64(exponent, 52); // shift by 52 bits to get mantissa

		// if exponent == 0
		const auto expEqualsZero = int4CompareEquals(exponent, _mm256_setzero_si256());

		// AND bits & MANTISSA_MASK > 0
		auto bitsAndMantissa = int4And(bits, MANTISSA_MASK);
		bitsAndMantissa = int4CompareGreaterThan(bitsAndMantissa, _mm256_setzero_si256());

		// If exponent == 0 && bits & MANTISSA_MASK set to true
		const auto invalid1 = int4And(expEqualsZero, bitsAndMantissa);

		// Similarly if exponent == 0x7ff it is invalid as well
		const auto invalid2 = int4CompareEquals(exponent, int4Set(0x7ff));

		// Combine both with OR operation
		// This will return for each element in the vector if they are invalid
		const auto invalid = int4Or(invalid1, invalid2);

		// Negate - i.e. XOR with 0xFFFFFFFFFFFFFFFF == int64_t(-1) == UINT64_MAX
		return int4Xor(invalid, int4Set(UINT64_MAX));
	}

	inline auto valuesInteger(const __m256d& x) {
		// Thankfully functionality for extracting fractional part is already implemented
		const auto integer = _mm256_round_pd(x, _MM_FROUND_TRUNC);
		const auto fraction = _mm256_sub_pd(x, integer);

		// Now we only need to check if fraction == 0.0
		const auto result = _mm256_cmp_pd(fraction, _mm256_setzero_pd(), _CMP_EQ_OQ);

		// Convert to "bool" array
		return double4ToInt4(result);
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

	inline void TestValuesIntegerFn() {
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
}