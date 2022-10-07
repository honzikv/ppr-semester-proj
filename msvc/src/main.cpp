#include <oneapi/tbb.h>
#include <iostream>
#include <ostream>
#include "Devices.h"




int main(int argc, char** argv) {
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
		std::cout << "\t\tDevice Max Allocateable Memory: " << device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() <<
			std::endl;
		std::cout << "\t\tDevice Local Memory: " << device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl;
		std::cout << "\t\tDevice Available: " << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
	}


	int sum = oneapi::tbb::parallel_reduce(oneapi::tbb::blocked_range<int>(1, 101), 0,
		[](oneapi::tbb::blocked_range<int> const& r, int init) -> int {
			for (int v = r.begin(); v != r.end(); v++) {
				init += v;
			}
			return init;
		},
		[](int lhs, int rhs) -> int {
			return lhs + rhs;
		}
		);

	std::cout << std::to_string(sum) << std::endl;
}
