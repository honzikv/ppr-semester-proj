//
// Created by itznu on 10/17/2022.
//

#ifndef PROTOTYPE_STRUCTS_H
#define PROTOTYPE_STRUCTS_H


#include <thread>
#include <atomic>
#include <condition_variable>
#include <filesystem>

class Watchdog {
public:
    Watchdog();
    void IncrementCounter();

private:
    std::thread watchdogThread;

    // Workers increment this variable to signal that they are still alive
    // Alternatively, use vector of atomic variables, one per worker
    std::atomic<uint32_t> counter = 0;
    std::condition_variable startCondition;

    void OnTimerExpired();
    void WatchDogMain();
};

void Watchdog::WatchDogMain() {

}

void Watchdog::OnTimerExpired() {

}

void Watchdog::IncrementCounter() {

}

Watchdog::Watchdog() {

}

struct ChunkStats {
    uint32_t ValidItems;
    double Min;
    double Max;
    double Mean;
    double Var;
};



struct ChunkDistribution {
    std::vector<uint32_t> Histogram;
};

class DeviceExecCoordinator {

};

class HANDLE {};

class IOHandler {
private:
    HANDLE fileHandle;
    uint64_t basePosition;
    std::vector<bool> processedChunks;

public:
    explicit IOHandler(std::filesystem::path filePath);
    std::vector<uint32_t> GetNUnprocessedChunks(uint32_t n);
    void MarkBatchProcessed(const std::vector<int>& chunkIds);
};

IOHandler::IOHandler(std::filesystem::path filePath) {

}

std::vector<uint32_t> IOHandler::GetNUnprocessedChunks(uint32_t n) {
    return std::vector<uint32_t>();
}

void IOHandler::MarkBatchProcessed(const std::vector<int>& chunkIds) {

}


class JobScheduler {
public:
    void Run();
    JobScheduler(Watchdog& watchdog, IOHandler& ioHandler);

private:
    Watchdog& watchdog;
    std::vector<DeviceExecCoordinator> deviceExecCoordinators;
    IOHandler& ioHandler;

    void createJob(const std::vector<uint32_t>& chunkIds);
    void assignJob(DeviceExecCoordinator& deviceExecCoordinator);
};

void JobScheduler::Run() {

}

void JobScheduler::CreateJob() {

}

JobScheduler::JobScheduler() {

}

void JobScheduler::createJob() {

}

void JobScheduler::assignJob(Device& device) {

}

void JobScheduler::createJob(const std::vector<uint32_t>& chunkIds) {

}

JobScheduler::JobScheduler(Watchdog& watchdog, IOHandler& ioHandler) {

}

#endif //PROTOTYPE_STRUCTS_H
