#pragma once

#include <vector>

enum Distribution {
    GAUSSIAN,
    UNIFORM,
    EXPONENTIAL,
    POISSON,
};

struct JobResult {
public:
    Distribution distribution;

    double kurtosis = .0;
    double skewness = .0;

    /**
     * Distance
     */
    double distance = .0;

    bool finished = false;

    JobResult(Distribution distribution, double skewness, double kurtosis, double distance) {
        this->distribution = distribution;
        this->skewness = skewness;
        this->kurtosis = kurtosis;
        this->distance = distance;
    }

    JobResult() = default;
};

struct Job {

public:
    std::vector<uint32_t> chunks;

    Job(std::vector<uint32_t> chunks) : chunks(std::move(chunks)) {}

};
