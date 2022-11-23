#pragma once
#include <memory>
#include <vector>

#include "Job.h"

/**
 * \brief This class collects all job results and aggregates them to a single result
 */
class JobAggregator {

	std::mutex mutex;

public:
	explicit JobAggregator(const size_t nCoordinators) {
		coordinatorJobResults.resize(nCoordinators);
	}

	void writeProcessedJob(std::unique_ptr<Job> job, const size_t coordinatorIdx) {
		auto lock = std::scoped_lock(mutex);
		coordinatorJobResults[coordinatorIdx] = std::move(job);
	}

	void update() {
		auto lock = std::scoped_lock(mutex);
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
		auto lock = std::scoped_lock(mutex);
		return runningStats;
	}

private:
	std::vector<std::shared_ptr<Job>> coordinatorJobResults; // results from all coordinators
	RunningStats runningStats; // global running stats

};
