cmake_minimum_required(VERSION 3.23)
project(prototype)

set(CMAKE_CXX_STANDARD 17)

add_executable(prototype src/WorkerPool.h src/Main.cpp src/OpenCL/OpenCLDevice.h src/structs.h)
