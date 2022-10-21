#pragma once

#include <vector>
#include "../device/device_coordinator.h"


/**
 * Class for scheduling jobs in the application,
 * this is always run on main thread and assigns job to other threads
 */
class JobScheduler {

public:
    JobScheduler() {

    }

private:
    std::vector<DeviceCoordinator> coordinators;

    void findAllDevices() {
        // Initialize CPU as a device
        coordinators.emplace_back("CPU");

        auto platforms = cl::Platform::get();
    }
};
