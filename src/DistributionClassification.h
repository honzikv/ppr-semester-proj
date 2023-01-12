#pragma once
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <iomanip>
#include <numeric>
#include <sstream>
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
 * \brief Simple structure to hold results of classification
 */
struct ClassificationResult {
	size_t DistributionIdx;
	double Distance;
	bool Valid;

	ClassificationResult(const size_t distributionIdx, const double distance, const bool valid)
		: DistributionIdx(distributionIdx),
		  Distance(distance),
		  Valid(valid) {
	}
};


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

inline void prettyPrintStatsNumber(const double num, const std::string& name, std::ostream& output) {
	if (std::isnan(num) || std::isinf(num)) {
		output << name << ": " << "Value overflown during computation" << std::endl;
		return;
	}

	output << name << ": " << num << "\n";
}

/**
 * \brief Prints statistics of StatsAccumulator to the given output stream
 * \param statsAccumulator stats accumulator to print stats for
 * \param distance the distance between the distribution and the distribution that was used to generate the data
 * \param output output stream
 */
inline void printStats(const StatsAccumulator& statsAccumulator, const double distance, std::ostream& output) {
	prettyPrintStatsNumber(statsAccumulator.getMin(), "Min", output);
	prettyPrintStatsNumber(statsAccumulator.getMean(), "Mean", output);
	prettyPrintStatsNumber(statsAccumulator.getVariance(), "Variance", output);
	prettyPrintStatsNumber(statsAccumulator.getStandardDeviation(), "Standard Deviation", output);
	prettyPrintStatsNumber(statsAccumulator.getSkewness(), "Skewness", output);
	prettyPrintStatsNumber(statsAccumulator.getKurtosis(), "Kurtosis", output);
	output << "Integer-only distribution: " << (statsAccumulator.integerDistribution() ? "Yes" : "No") << "\n";
	prettyPrintStatsNumber(distance, "Distance from the classified distribution", output);
}

/**
 * \brief Performs classification on specified StatsAccumulator
 * \param statsAccumulator stats accumulator to classify
 * \return pair of ClassificationResult and notes during classification
 */
inline auto classifyStatsAccumulator(const StatsAccumulator& statsAccumulator) {
	const auto estimatedPt = std::make_pair(statsAccumulator.getSkewness(), statsAccumulator.getKurtosis());
	auto distributionPoints = std::vector{
		GAUSSIAN_DISTRIBUTION, EXPONENTIAL_DISTRIBUTION, UNIFORM_DISTRIBUTION, {0, 0}
	};

	auto classificationNotes = std::vector<std::string>{};

	if (!statsAccumulator.valid()) {
		classificationNotes.emplace_back("Computed statistics are not valid - some values are NaNs or infinity.");
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
		if (integersOnly && i == POISSON_IDX || integersOnly && i == UNIFORM_IDX ||
			!integersOnly && i != POISSON_IDX) {
			distances[i] = euclideanDistance2d(distributionPoints[i], estimatedPt);
			continue;
		}

		if (integersOnly && !(i == POISSON_IDX || i == UNIFORM_IDX)) {
			classificationNotes.emplace_back("Discarding " + DISTRIBUTION_STR_LUT.at(i) +
				" since only integers were detected in the data.");
			continue;
		}

		if (i == EXPONENTIAL_IDX && statsAccumulator.getMin() < 0) {
			classificationNotes.emplace_back(
				"Discarding exponential distribution from the classification as the minimum value in the data is less than zero.");
			continue;
		}

		classificationNotes.emplace_back("Discarding " + DISTRIBUTION_STR_LUT.at(i) +
			" distribution from the classification since the data contains non-integer values.");
	}

	// Get index of the smallest distance
	const auto closestDistributionIdx = static_cast<size_t>(
		std::distance(std::begin(distances), std::min_element(distances.begin(), distances.end()))
	);

	const auto distance = distances[closestDistributionIdx];

	return std::make_pair(ClassificationResult{
		                      closestDistributionIdx,
		                      distance,
		                      statsAccumulator.valid()
	                      }, classificationNotes);
}

/**
 * \brief Classifies given distribution from passed StatsAccumulator object and writes results to given outputstream
 * \param statsAccumulator stats accumulator to classify
 * \param output output stream to write to
 */
inline void classifyDistribution(const StatsAccumulator& statsAccumulator, std::ostream& output = std::cout) {
	output << "\nResults" << "\n";
	output << "-------" << "\n";

	const auto [classificationResult, classificationNotes] = classifyStatsAccumulator(statsAccumulator);
	for (const auto& note : classificationNotes) {
		output << "- " << note << "\n" << "\n";
	}

	// Lookup the distribution name
	const auto& distributionName = DISTRIBUTION_STR_LUT.at(classificationResult.DistributionIdx);

	// And finally print to the stdout
	output << "The distribution was classified as: \"" << distributionName << "\"" << "\n" << "\n";
	printStats(statsAccumulator, classificationResult.Distance, output);
	output << std::endl;
}
