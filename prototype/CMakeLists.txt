cmake_minimum_required(VERSION 3.23)
project(prototype)

set(CMAKE_CXX_STANDARD 17)

add_executable(prototype
        src/Main.cpp src/monitoring/watchdog.h
        src/job/job_scheduler.cpp
        src/job/job_scheduler.h
        src/device/device_coordinator.cpp
        src/device/device_coordinator.h
        src/job/job.cpp src/job/job.h src/io/io_handler.cpp src/io/io_handler.h)
