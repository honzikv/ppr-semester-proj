#pragma once
#include <algorithm>
#include <vector>
#include <cmath>


struct JobResult {
	bool success;
	double mean, variance, skewness, kurtosis;

	inline JobResult():
		mean(std::nan("0")),
		variance(std::nan("0")), skewness(std::nan("0")), kurtosis(std::nan("0")) {
	}

	inline explicit JobResult(const bool success):
		success(success), mean(std::nan("0")),
		variance(std::nan("0")), skewness(std::nan("0")), kurtosis(std::nan("0")) {
	}

	inline JobResult(const bool success, const double mean, const double variance, const double skewness,
	                 const double kurtosis) :
		success(success), mean(mean), variance(variance), skewness(skewness), kurtosis(kurtosis) {
	}

};

/**
 * \brief Contains information about the processed job - i.e. which chunks to process, results of the processing and so on
 */
struct Job {
	/**
	 * \brief List of chunks to process
	 */
	std::vector<uint32_t> chunksToProcess;
	JobResult result;
	bool success = false;

	inline explicit Job(std::vector<uint32_t> chunksToProcess) : chunksToProcess(std::move(chunksToProcess)) {
	}
};
