cmake_minimum_required(VERSION 3.7)

# Set project name
project(PPR VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
include_directories(src)

add_executable(${PROJECT_NAME} src/main.cpp src/Devices.h src/ArgParsing.h)

find_package(TBB REQUIRED)
find_package(OpenCL REQUIRED)
target_link_libraries(${PROJECT_NAME} TBB::tbb)
target_link_libraries(${PROJECT_NAME} OpenCL::OpenCL)
