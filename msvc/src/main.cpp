#include <cassert>
#include <oneapi/tbb.h>
#include <iostream>
#include <ostream>
#include "Devices.h"


int main(int argc, char** argv) {
	//TestOpenCl();

	// parseArgs(argc, argv);

	const auto devices = FindAllAvailableDevices();

	auto device_id = 0;
	for (const auto& device : devices) {
		std::cout << "\tDevice " << device_id++ << ": " << std::endl;
		std::cout << "\t\tDevice Name: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
		std::cout << "\t\tDevice Type: " << device.getInfo<CL_DEVICE_TYPE>();
		std::cout << " (GPU: " << CL_DEVICE_TYPE_GPU << ", CPU: " << CL_DEVICE_TYPE_CPU << ")" << std::endl;
		std::cout << "\t\tDevice Vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
		std::cout << "\t\tDevice Max Compute Units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
		std::cout << "\t\tDevice Global Memory: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl;
		std::cout << "\t\tDevice Max Clock Frequency: " << device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << std::endl;
		std::cout << "\t\tDevice Max Allocatable Memory: " << device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() <<
			std::endl;
		std::cout << "\t\tDevice Local Memory: " << device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl;
		std::cout << "\t\tDevice Available: " << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
	}

	const int N1 = 1000, N2 = 1000;
	oneapi::tbb::enumerable_thread_specific<int> ets;
	oneapi::tbb::parallel_for(0, N1, [&ets](int i) {
		// Set a thread specific value
		ets.local() = i;
		// Run the second parallel loop in an isolated region to prevent the current thread
		// from taking tasks related to the outer parallel loop.
		oneapi::tbb::this_task_arena::isolate([] {
			oneapi::tbb::parallel_for(0, 100, [](int j) {
				std::cout << j << std::endl;
			});
		});
		std::cout << "Hello TBB" << std::endl;
		assert(ets.local() == i); // Valid assertion
	});
	return 0;
}
