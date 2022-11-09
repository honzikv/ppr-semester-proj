#pragma once
#include <algorithm>
#include <vector>


struct JobResult {

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