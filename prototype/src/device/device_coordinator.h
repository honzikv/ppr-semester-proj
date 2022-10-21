#pragma once

#include <string>
#include <memory>
#include <utility>
#include "../job/job.h"
#include "../monitoring/watchdog.h"
#include "../job/job_scheduler.h"

class DeviceCoordinator {

protected:
    std::string deviceName;

    bool working = false;

    std::shared_ptr<Job> job;

    Watchdog& watchdog;

    JobScheduler& jobScheduler;

public:
    explicit DeviceCoordinator(std::string deviceName, Watchdog& watchdog, JobScheduler& jobScheduler) : deviceName(
            std::move(deviceName)), watchdog(watchdog), jobScheduler(jobScheduler) {}

    void assignJob(std::shared_ptr<Job> jobPtr) {
        this->job = std::move(jobPtr);

        // Set state to working
        working = true;

        processJob();
    }

    [[nodiscard]] bool available() const {
        return !working;
    }



private:
    void processJob() {
        watchdog.Kick();

        // Load the data
        auto data =


    }

};
