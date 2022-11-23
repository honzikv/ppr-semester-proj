#pragma once
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "RunningStats.h"


using Point2D = std::pair<double, double>;

// Definition of points in (skewness, kurtosis) 2D space
static constexpr Point2D GAUSSIAN_DISTRIBUTION = std::make_pair(.0, .0);
static constexpr Point2D EXPONENTIAL_DISTRIBUTION = std::make_pair(2.0, 6.0);
static constexpr Point2D UNIFORM_DISTRIBUTION = std::make_pair(.0, -5.0 / 6.0);

static const auto DISTRIBUTIONS = std::vector{GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION, UNIFORM_DISTRIBUTION};
static const auto DISTRIBUTION_STR_LUT = std::vector{ "Gaussian", "Exponential", "Uniform" };

inline auto euclideanDistance(std::pair<double, double> a, std::pair<double, double> b) {
	const auto [x1, y1] = a;
	const auto [x2, y2] = b;
	return std::hypot(x1 - x2, y1 - y2);
}

inline void printStats(const RunningStats& runningStats) {
	std::cout << "Mean: " << runningStats.getMean() << "\n";
	std::cout << "Variance: " << runningStats.getVariance() << "\n";
	std::cout << "Standard Deviation: " << runningStats.getStandardDeviation() << "\n";
	std::cout << "Skewness: " << runningStats.getSkewness() << "\n";
	std::cout << "Kurtosis: " << runningStats.getKurtosis() << std::endl;
}

inline void classifyDistribution(const RunningStats& runningStats) {
	const auto estimatedPt = std::make_pair(runningStats.getSkewness(), runningStats.getKurtosis());

	std::cout << "Results" << "\n";

	if (runningStats.integerDistribution()) {
		// If running stats has only integers we know that the distribution is Poisson
		std::cout << "Classified distribution as: \"Poisson\"" << "\n" << "\n";
		std::cout << "Distribution comprises only integers (no decimal numbers were detected)" << "\n";
		std::cout << "Therefore classifying as \"Poisson\"" << "\n";
		printStats(runningStats);
		return;
	}

	// Compute distance from each distribution
	// 0 - gaussian / poisson, 1 - exp, 2- uniform
	auto distances = std::vector<double>();
	for (const auto& distribution : DISTRIBUTIONS) {
		distances.push_back(euclideanDistance(distribution, estimatedPt));
	}

	// Get index of the smallest distance
	const auto closestDistributionIdx =
		std::distance(std::begin(distances), std::min_element(distances.begin(), distances.end()));

	// Lookup the distribution name
	const auto distributionName = DISTRIBUTION_STR_LUT.at(static_cast<size_t>(closestDistributionIdx));

	// And finally print to the stdout
	std::cout << "Classified distribution as: \"" << distributionName << "\"" << "\n" << "\n";
	printStats(runningStats);
}
