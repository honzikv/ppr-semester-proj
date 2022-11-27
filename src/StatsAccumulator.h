#pragma once
#include <iostream>


class Avx2StatsAccumulator;

class StatsAccumulator {
	friend class Avx2StatsAccumulator;

	size_t n = 0; // Number of items processed

	/**
	 * Moments in the distribution
	 */
	double m1 = .0, m2 = .0, m3 = .0, m4 = .0;

	bool isIntegerDistribution = true;

public:
	StatsAccumulator() = default;

	StatsAccumulator(const size_t n, const double m1, const double m2, const double m3, const double m4, const bool isIntegerDistribution);

	void push(const double x);

	[[nodiscard]] size_t getN() const;

	[[nodiscard]] double getMean() const;

	[[nodiscard]] double getVariance() const {
		return m2 / (n - 1.0);
	}

	[[nodiscard]] double getStandardDeviation() const {
		return std::sqrt(getVariance());
	}

	[[nodiscard]] double getSkewness() const {
		return std::sqrt(static_cast<double>(n)) * m3 / std::pow(m2, 1.5);
	}

	[[nodiscard]] double getKurtosis() const {
		return static_cast<double>(n) * m4 / (m2 * m2) - 3.0;
	}

	StatsAccumulator operator+(const StatsAccumulator& other) const;

	StatsAccumulator& operator+=(const StatsAccumulator& rhs);

	[[nodiscard]] bool integerDistribution() const {
		return isIntegerDistribution;
	}

	/**
	 * \brief 
	 * \return true if the values are within reasonable precision
	 */
	bool valid() const;

	/**
	 * \brief Debug
	 */
	void debugPrint() const;
};
