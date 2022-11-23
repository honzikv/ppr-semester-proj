#pragma once
#include <memory>
#include <vector>

#include "Job.h"

/**
 * \brief This class collects all job results and aggregates them to a single result
 */
class JobAggregator {

public:
	explicit JobAggregator(const size_t nCoordinators) {
		coordinatorJobResults.resize(nCoordinators);
	}

	void writeProcessedJob(std::unique_ptr<Job> job, const size_t coordinatorIdx) {
		coordinatorJobResults[coordinatorIdx] = std::move(job);
	}

	void update() {
		for (const auto& job : coordinatorJobResults) {
			if (job != nullptr) {
				runningStats += job->Result;
			}
		}

		// Reset the job results
		for (auto& job : coordinatorJobResults) {
			job = nullptr;
		}
	}

	[[nodiscard]] auto getResult()  {
		return runningStats;
	}

private:
	std::vector<std::shared_ptr<Job>> coordinatorJobResults; // results from all coordinators
	RunningStats runningStats; // global running stats

};
