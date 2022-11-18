#pragma once
#include <cmath>

class RunningStats {

	size_t n = 0; // Number of items processed

	/**
	 * Moments in the distribution
	 */
	double m1 = .0, m2 = .0, m3 = .0, m4 = .0;

public:
	inline auto push(const double x) {
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

	[[nodiscard]] inline auto getN() const {
		return n;
	}

	[[nodiscard]] inline auto getMean() const {
		return m1;
	}

	[[nodiscard]] inline auto getVariance() const {
		return m2 / (n - 1.0);
	}

	[[nodiscard]] inline auto getStandardDeviation() const {
		return std::sqrt(getVariance());
	}

	[[nodiscard]] inline auto getSkewness() const {
		return std::sqrt(static_cast<double>(n)) * m3 / std::pow(m2, 1.5);
	}

	[[nodiscard]] inline auto getKurtosis() const {
		return static_cast<double>(n) * m4 / (m2 * m2) - 3.0;
	}

	inline auto operator+(const RunningStats& other) const {
		RunningStats result;
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
		return result;
	}

	inline auto& operator+=(const RunningStats& rhs) {
		const auto combined = *this + rhs;
		*this = combined;
		return *this;
	}
};
