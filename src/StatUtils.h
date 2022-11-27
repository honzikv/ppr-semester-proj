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

	/**
	 * \brief Merges results in the passed vector in pairs
	 *		  E.g. for array [1, 2, 3, 4, 5] -> [(1, 2), (3, 4), (5)] -> [(1, 2, 3, 4), (5)] -> (1, 2, 3, 4, 5)
	 * \param items reference to the vector with items. This array is always modified
	 * \param filterInvalid whether to throw away invalid items
	 * \return Copy of the first item in the vector
	 */
	static auto mergePairwise(const std::vector<StatsAccumulator>& items, const bool filterInvalid = true) {
		auto filtered = std::vector<StatsAccumulator>();
		if (filterInvalid) {
			for (const auto& item : items) {
				if (item.valid()) {
					filtered.push_back(item);
				}
			}
		}
		else {
			filtered = items;
		}

		// NOLINT(clang-diagnostic-unused-function)
		auto itemsToProcess = filtered.size();
		while (true) {
			const auto nPairs = itemsToProcess / 2 + itemsToProcess % 2;
			for (auto i = 0ULL; i < nPairs; i += 1) {
				filtered[i] = i * 2 + 1 < itemsToProcess ? filtered[i * 2] + filtered[i * 2 + 1] : filtered[i * 2];
			}

			if (nPairs == itemsToProcess) {
				break;
			}

			itemsToProcess = nPairs;
		}

		return filtered[0];
	}

	static auto mergeLeftToRight(const std::vector<StatsAccumulator>& items, const bool filterInvalid = true) {
		auto filtered = std::vector<StatsAccumulator>();
		if (filterInvalid) {
			for (const auto& item : items) {
				if (item.valid()) {
					filtered.push_back(item);
				}
			}
		}
		else {
			filtered = items;
		}

		// If all items are corrupted we return the first one
		// The classifier checks whether the result is valid and informs the user if it is not
		if (filtered.empty()) {
			return items[0];
		}

		auto& result = filtered[0];
		for (auto i = 1ULL; i < filtered.size(); i += 1) {
			result += filtered[i];
		}

		return result;
	}
}
