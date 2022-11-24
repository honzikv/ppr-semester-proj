#pragma once

#include <cmath>

/**
 * \brief Simple utils namespace for statistics and floating point computation
 */
namespace StatUtils {

	/**
	 * \brief Boolean check whether int(x) == double(x)
	 * \param x value of x
	 * \return true if x is an integer, false otherwise
	 */
	inline bool isValueInteger(const double x) {
		double intPart;
		return std::modf(x, &intPart) == 0.0;
	}

	/**
	 * \brief Checks whether double is FP_ZERO or FP_NORMAL
	 * \param x value of x
	 * \return true if x is FP_ZERO or FP_NORMAL, false otherwise
	 */
	inline bool valueNormalOrZero(const double x) {
		const auto fpClassification = std::fpclassify(x);
		return fpClassification == FP_ZERO || fpClassification == FP_NORMAL;
	}
}
