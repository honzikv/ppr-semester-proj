#include "ClDeviceCoordinator.h"
#include <fstream>

void ClDeviceCoordinator::onProcessJob() {
	const auto deviceBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, maxNumberOfChunks);
	dataLoader.loadJobDataIntoDeviceBuffer(*currentJob, maxHostChunks, commandQueue, deviceBuffer);
	
}
