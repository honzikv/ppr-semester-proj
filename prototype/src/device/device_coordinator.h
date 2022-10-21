#pragma once

#include <string>
#include <memory>
#include "../job/job.h"

class DeviceCoordinator {

public:
    explicit DeviceCoordinator(std::string deviceName) : deviceName(std::move(deviceName)) {}

    void assignJob(std::shared_ptr<Job> jobPtr) {
        this->job = jobPtr;

        // Set state to working
        working = true;

        //
    }

    [[nodiscard]] bool available() const {
        return !working;
    }

private:
    std::string deviceName;

    bool working = false;

    std::shared_ptr<Job> job;
};
