cmake_minimum_required(VERSION 3.23)
project(clSandbox)

set(CMAKE_CXX_STANDARD 17)

add_executable(clSandbox main.cpp src/ClKernels.h src/clTest.h src/gpu.h)
find_package(OpenCL REQUIRED)
target_link_libraries(clSandbox OpenCL::OpenCL)
