#pragma once
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>
#include "StatsAccumulator.h"


using Point2D = std::pair<double, double>;

// Definition of points in (skewness, kurtosis) 2D space
static constexpr Point2D GAUSSIAN_DISTRIBUTION = std::make_pair(.0, .0);
static constexpr Point2D EXPONENTIAL_DISTRIBUTION = std::make_pair(2.0, 6.0);
static constexpr Point2D UNIFORM_DISTRIBUTION = std::make_pair(.0, -5.0 / 6.0);

static const auto DISTRIBUTIONS = std::vector{GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION, UNIFORM_DISTRIBUTION};
static const auto DISTRIBUTION_STR_LUT = std::vector{"Gaussian", "Exponential", "Uniform"};

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
inline void printStats(const StatsAccumulator& statsAccumulator, std::ostream& output) {
	output << "Mean: " << statsAccumulator.getMean() << "\n";
	output << "Variance: " << statsAccumulator.getVariance() << "\n";
	output << "Standard Deviation: " << statsAccumulator.getStandardDeviation() << "\n";
	output << "Skewness: " << statsAccumulator.getSkewness() << "\n";
	output << "Kurtosis: " << statsAccumulator.getKurtosis() << std::endl;
}

/**
 * \brief Classifies given distribution from passed StatsAccumulator object and writes results to given outputstream
 * \param statsAccumulator stats accumulator to classify
 * \param output output stream to write to 
 */
inline void classifyDistribution(const StatsAccumulator& statsAccumulator, std::ostream& output = std::cout) {
	const auto estimatedPt = std::make_pair(statsAccumulator.getSkewness(), statsAccumulator.getKurtosis());

	output << "\nResults" << "\n";
	output << "-------" << "\n";

	if (!statsAccumulator.valid()) {
		output << "Some values of the distributions have invalid values (likely due to underflow / overflow) - the classification may be imprecise!" << std::endl;
	}

	if (statsAccumulator.integerDistribution()) {
		// If running stats has only integers we know that the distribution is Poisson
		output << "Classified distribution as: \"Poisson\"" << "\n" << "\n";
		output << "Distribution comprises only integers (no decimal numbers were detected)" << "\n";
		output << "Therefore classifying as \"Poisson\"" << "\n";
		printStats(statsAccumulator, output);
		return;
	}

	// Compute distance from each distribution
	// 0 - gaussian / poisson, 1 - exp, 2- uniform
	auto distances = std::vector<double>();
	for (const auto& distribution : DISTRIBUTIONS) {
		distances.push_back(euclideanDistance2d(distribution, estimatedPt));
	}

	// Get index of the smallest distance
	const auto closestDistributionIdx =
		std::distance(std::begin(distances), std::min_element(distances.begin(), distances.end()));

	// Lookup the distribution name
	const auto distributionName = DISTRIBUTION_STR_LUT.at(static_cast<size_t>(closestDistributionIdx));

	// And finally print to the stdout
	output << "Classified distribution as: \"" << distributionName << "\"" << "\n" << "\n";
	printStats(statsAccumulator, output);
}
