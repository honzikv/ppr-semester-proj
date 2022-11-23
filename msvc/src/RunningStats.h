#pragma once
#include <cmath>
#include <iostream>

class Avx2RunningStats;

class RunningStats {
	friend class Avx2RunningStats;

	size_t n = 0; // Number of items processed

	/**
	 * Moments in the distribution
	 */
	double m1 = .0, m2 = .0, m3 = .0, m4 = .0;

	bool isIntegerDistribution = true;

public:
	void push(const double x) {
		// Check whether x is FP_NORMAL
		const auto fpType = std::fpclassify(x);
		if (fpType != FP_ZERO && fpType != FP_NORMAL) {
			return;
		}
		
		// Check if x is an integer
		isIntegerDistribution = isIntegerDistribution && isInt(x);

		const auto n1 = n;
		n += 1;

		const auto delta = x - m1;
		const auto deltaN = delta / n;
		const auto deltaNSquared = deltaN * deltaN;
		const auto term1 = delta * deltaN * n1;
		m1 += deltaN;
		m4 += term1 * deltaNSquared * (n * n - 3 * n + 3) + 6 * deltaNSquared * m2 - 4 * deltaN * m3;
		m3 += term1 * deltaN * (n - 2) - 3 * deltaN * m2;
		m2 += term1;
	}

	[[nodiscard]] size_t getN() const {
		return n;
	}

	[[nodiscard]] double getMean() const {
		return m1;
	}

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

	auto operator+(const RunningStats& other) const {
		if (other.n == 0) {
			return *this;
		}

		if (n == 0) {
			return other;
		}

		auto result = RunningStats();
		result.n = n + other.n;

		const auto delta = other.m1 - m1;
		const auto delta2 = delta * delta;
		const auto delta3 = delta2 * delta;
		const auto delta4 = delta2 * delta2;
		const auto mean = (m1 * n + other.m1 * other.n) / result.n;
		result.m1 = mean;
		result.m2 = m2 + other.m2 + delta2 * n * other.n / result.n;
		result.m3 = m3 + other.m3 + delta3 * n * other.n * (n - other.n) / (result.n * result.n) +
			3.0 * delta * (n * other.m2 - other.n * m2) / result.n;
		result.m4 = m4 + other.m4 + delta4 * n * other.n * (n * n - n * other.n + other.n * other.n) /
			(result.n * result.n * result.n) +
			6.0 * delta2 * (n * n * other.m2 + other.n * other.n * m2) / (result.n * result.n) +
			4.0 * delta * (n * other.m3 - other.n * m3) / result.n;
		
		result.isIntegerDistribution = isIntegerDistribution && other.isIntegerDistribution;

		// debugPrint();

		return result;
	}

	auto& operator+=(const RunningStats& rhs) {
		const auto combined = *this + rhs;
		*this = combined;
		return *this;
	}

	[[nodiscard]] bool integerDistribution() const {
		return isIntegerDistribution;
	}

	[[nodiscard]] bool isInt(const double x) const {
		double intPart;
		return std::modf(x, &intPart) == 0.0;
	}

	/**
	 * \brief Debug
	 */
	void debugPrint() const {
		std::cout << "\nRunningStats" << std::endl;
		std::cout << "M1: " << m1 << std::endl;
		std::cout << "M2: " << m2 << std::endl;
		std::cout << "M3: " << m3 << std::endl;
		std::cout << "M4: " << m4 << std::endl;
		std::cout << "n: " << n << std::endl;

		std::cout << "Mean: " << getMean() << std::endl;
		std::cout << "Skewness: " << getSkewness() << std::endl;
		std::cout << "Kurtosis: " << getKurtosis() << std::endl;
		std::cout << std::endl;
	}
};
