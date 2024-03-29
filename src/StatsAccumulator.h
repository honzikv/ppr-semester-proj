#pragma once
#include <iostream>

// Forward declaration for friend class
class Avx2StatsAccumulator;
// class StatsMerger;

/**
 * \brief Object for accumulating distribution statistics
 */
class StatsAccumulator {
	friend class Avx2StatsAccumulator;
	// friend class StatsMerger;

	/**
	 * \brief Number of processed (valid) items
	 */
	size_t n = 0;

	/**
	 * Moments of the distribution
	 */
	double m1 = .0, m2 = .0, m3 = .0, m4 = .0;

	/**
	 * \brief Flag to signal whether the distribution comprises only integers
	 */
	bool isIntegerDistribution = true;

	/**
	 * \brief Minimum value in the distribution
	 */
	double minVal = std::numeric_limits<double>::infinity();

	/**
	 * \brief When two accumulators are merged, this signals that the merge was not valid
	 */
	bool numericalErrorWhileMerging = false;

public:
	StatsAccumulator();

	/**
	 * \brief Creates new StatsAccumulator instance with predefined values
	 * \param n number of items processed
	 * \param m1 the first moment
	 * \param m2 the second moment
	 * \param m3 the third moment
	 * \param m4 the fourth moment
	 * \param isIntegerDistribution whether the distribution comprises only integer values 
	 */
	StatsAccumulator(size_t n, double m1, double m2, double m3, double m4, bool isIntegerDistribution, double min);

	/**
	 * \brief Pushes new value to the accumulator
	 * \param x value to be pushed
	 */
	void push(double x);

	/**
	 * \brief Returns number of items
	 * \return Number of items
	 */
	[[nodiscard]] size_t getN() const;

	/**
	 * \brief Returns mean of the distribution 
	 * \return Mean of the distribution
	 */
	[[nodiscard]] double getMean() const;

	/**
	 * \brief Returns minimum of the distribution
	 * \return Minimum of the distribution
	 */
	[[nodiscard]] double getMin() const;

	/**
	 * \brief Returns variance of the distribution
	 * \return Variance of the distribution
	 */
	[[nodiscard]] double getVariance() const;

	/**
	 * \brief Returns standard deviation of the distribution
	 * \return Standard deviation of the distribution
	 */
	[[nodiscard]] double getStandardDeviation() const;

	/**
	 * \brief Returns skewness of the distribution
	 * \return Skewness of the distribution
	 */
	[[nodiscard]] double getSkewness() const;

	/**
	 * \brief Returns kurtosis of the distribution
	 * \return Kurtosis of the distribution
	 */
	[[nodiscard]] double getKurtosis() const;

	[[nodiscard]] bool numericallyErroredWhileMerging() const;

	/**
	 * \brief Adds another accumulator to this one
	 * \param other Other accumulator to be added
	 * \return Result of the addition
	 */
	StatsAccumulator operator+(StatsAccumulator& other);

	/**
	 * \brief Adds another accumulator to this one
	 * \param rhs right hand side
	 * \return result of the addition
	 */
	StatsAccumulator& operator+=(StatsAccumulator& rhs);

	/**
	 * \brief Returns whether the distribution comprises only integer values
	 * \return true if the distribution comprises only integer values, false otherwise
	 */
	[[nodiscard]] bool integerDistribution() const;

	/**
	 * \brief Returns whether the distribution is valid - i.e. no underflow or overflow occurred during the computation
	 * \return true if the values are within reasonable precision
	 */
	[[nodiscard]] bool valid() const;

	/**
	 * \brief Prints inner state of the accumulator, used mainly for debugging
	 */
	void debugPrint() const;
};
