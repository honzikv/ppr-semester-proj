#pragma once

#include <cmath>
#include <vector>
#include "StatsAccumulator.h"

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

	static auto mergePairwise(std::vector<StatsAccumulator>& results) {
		auto itemsToProcess = results.size();
		while (true) {
			const auto nPairs = itemsToProcess / 2 + itemsToProcess % 2;
			for (auto i = 0ULL; i < nPairs; i += 1) {
				results[i] = i * 2 + 1 < itemsToProcess ? results[i * 2] + results[i * 2 + 1] : results[i * 2];
			}

			if (nPairs == itemsToProcess) {
				break;
			}

			itemsToProcess = nPairs;
		}

		return results[0];
	}

	static auto mergeLeftToRight(std::vector<StatsAccumulator>& results) {
		auto& result = results[0];
		for (auto i = 0ULL; i < results.size(); i += 1) {
			result += results[i];
		}

		return result;
	}
}
