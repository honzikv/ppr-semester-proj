#pragma once

#include <condition_variable>
#include <utility>
#include <vector>
#include <unordered_map>
#include "OpenCL/OpenCLDevice.h"

/**
 * Custom implementation of thread pool data structure.
 * This is not a thread pool per se - it is intended to be used with a single thread that will be suspended
 * if there are no workers available
 */
class WorkerPool {


public:
    /**
     * @brief Construct a new Worker Pool object - one worker per OpenCL clDevice
     * @param devices list of all devices
     */
    explicit WorkerPool(std::vector<OpenCLDevice> devices) : clDevices(std::move(devices)) {



    }

    bool shutdown = false;  // Flag for shutting down the thread pool

    std::vector<OpenCLDevice> clDevices; // List of all OpenCL devices
};