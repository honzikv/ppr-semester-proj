cmake_minimum_required(VERSION 3.7)

# Set project name
project(PPR VERSION 0.1.0 LANGUAGES CXX)
add_executable(ppr_solver src/main.cpp src/Devices.h src/argParsing.h)

find_package(OpenCL REQUIRED)
target_link_libraries(ppr_solver OpenCL::OpenCL)
include_directories(src)