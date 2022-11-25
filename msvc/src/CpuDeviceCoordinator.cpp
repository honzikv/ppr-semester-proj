#include <oneapi/tbb.h>
#include <fstream>

#include "CpuDeviceCoordinator.h"


void CpuDeviceCoordinator::onProcessJob() {
	std::cout << "OnProcessJob CPU" << std::endl;

}
