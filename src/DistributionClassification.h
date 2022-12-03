#pragma once
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>
#include "StatsAccumulator.h"


using Point2D = std::pair<double, double>;

// Definition of points in (skewness, kurtosis) 2D space
constexpr Point2D GAUSSIAN_DISTRIBUTION = std::make_pair(.0, .0);
constexpr Point2D EXPONENTIAL_DISTRIBUTION = std::make_pair(2.0, 6.0);
constexpr Point2D UNIFORM_DISTRIBUTION = std::make_pair(.0, -5.0 / 6.0);
constexpr auto EXPONENTIAL_IDX = 1;
constexpr auto UNIFORM_IDX = 2;
constexpr auto POISSON_IDX = 3;

const auto DISTRIBUTION_STR_LUT = std::vector<std::string>{"Gaussian", "Exponential", "Uniform", "Poisson"};

/**
 * \brief Compute euclidean distance between two 2D points
 * \param a point A - x,y coordinates
 * \param b point B - x,y coordinates
 * \return euclidean distance between two points
 */
inline auto euclideanDistance2d(std::pair<double, double> a, std::pair<double, double> b) {
	const auto [x1, y1] = a;
	const auto [x2, y2] = b;
	return std::hypot(x1 - x2, y1 - y2);
}

/**
 * \brief Prints statistics of StatsAccumulator to the given output stream
 * \param statsAccumulator stats accumulator to print stats for
 * \param output output stream
 */
inline void printStats(const StatsAccumulator& statsAccumulator, const double distance, std::ostream& output) {
	output << "Min: " << statsAccumulator.getMin() << "\n";
	output << "Mean: " << statsAccumulator.getMean() << "\n";
	output << "Variance: " << statsAccumulator.getVariance() << "\n";
	output << "Standard Deviation: " << statsAccumulator.getStandardDeviation() << "\n";
	output << "Skewness: " << statsAccumulator.getSkewness() << "\n";
	output << "Kurtosis: " << statsAccumulator.getKurtosis() << "\n";
	output << "Integer only distribution: " << (statsAccumulator.integerDistribution() ? "Yes" : "No") << "\n";
	output << "Distance from the classified distribution: " << distance << "\n";
}

/**
 * \brief Classifies given distribution from passed StatsAccumulator object and writes results to given outputstream
 * \param statsAccumulator stats accumulator to classify
 * \param output output stream to write to 
 */
inline void classifyDistribution(const StatsAccumulator& statsAccumulator, std::ostream& output = std::cout) {
	const auto estimatedPt = std::make_pair(statsAccumulator.getSkewness(), statsAccumulator.getKurtosis());
	auto distributionPoints = std::vector{
		GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION, UNIFORM_DISTRIBUTION, {0, 0}
	};

	output << "\nResults" << "\n";
	output << "-------" << "\n";

	if (!statsAccumulator.numericallyErroredWhileMerging()) {
		output <<
			"- A numerical error (floating point overflow / underflow) occurred while merging the distributions - the classification may be imprecise!"
			<< "\n" << "\n";
	}

	// Compute poisson distribution skewness and kurtosis
	distributionPoints[POISSON_IDX] = std::abs(statsAccumulator.getMean()) < 1e-5
		                                  ? std::make_pair(.0, .0)
		                                  : std::make_pair(1. / std::sqrt(statsAccumulator.getMean()),
		                                                   1. / statsAccumulator.getMean());

	const auto integersOnly = statsAccumulator.integerDistribution();
	// Compute distances
	auto distances = std::vector<double>(4, {std::numeric_limits<double>::infinity()});
	for (auto i = 0ULL; i < distributionPoints.size(); i += 1) {
		if (integersOnly && i == POISSON_IDX ||
			!integersOnly && i != POISSON_IDX) {
			distances[i] = euclideanDistance2d(distributionPoints[i], estimatedPt);
		}

		if (integersOnly && !(i == POISSON_IDX || i == UNIFORM_IDX)) {
			output << "- Discarding " + DISTRIBUTION_STR_LUT.at(i) +
				" distribution from the classification as the distribution contains only integers" << "\n" << "\n";
			continue;
		}

		if (i == EXPONENTIAL_IDX && statsAccumulator.getMin() < 0) {
			output <<
				"- Discarding exponential distribution from the classification as the minimum value is less than zero"
				<< "\n" << "\n";
			continue;
		}

		output << "- Discarding " + DISTRIBUTION_STR_LUT.at(i) + " distribution from the classification as the distribution contains real values" << "\n" << "\n";
	}

	// Get index of the smallest distance
	const auto closestDistributionIdx =
		std::distance(std::begin(distances), std::min_element(distances.begin(), distances.end()));

	// Lookup the distribution name
	const auto& distributionName = DISTRIBUTION_STR_LUT.at(static_cast<size_t>(closestDistributionIdx));

	const auto distance = distances[closestDistributionIdx];

	// And finally print to the stdout
	output << "The distribution was classified as: \"" << distributionName << "\"" << "\n" << "\n";
	printStats(statsAccumulator, distance, output);
	output << std::endl;
}
