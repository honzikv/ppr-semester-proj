#include "Arghandling.h"
#include "DistributionClassification.h"
#include "JobScheduler.h"
#include <fstream>

#include "OpenClTest.h"

int main(int argc, char** argv) {
	auto args = parseArguments(argc, argv);
	auto processingInfo = validateArguments(args);
	auto jobScheduler = JobScheduler(processingInfo);
	auto result = jobScheduler.run();
	
	// Classify the result
	classifyDistribution(result);
	//
	// // VectorizationUtils::testValuesInvalidFn();
	// // VectorizationUtils::TestValuesIntegerFn();
	//
	// std::cout << "TEST SINGLE THREAD" << std::endl;
	// auto filePath = processingInfo.DistFilePath;
	//
	// auto runningStats = RunningStats();
	//
	// auto buffer = std::vector<double>();
	// auto file = std::ifstream(filePath, std::ios::binary);
	//
	// auto bufferSize = 8 * 1024 * 1024;
	// auto fileSize = fs::file_size(filePath);
	// auto nReads = static_cast<size_t>(ceil(fileSize / bufferSize));
	//
	// auto bytesRemaining = fileSize;
	// for (auto i = 0; i < nReads; i += 1) {
	// 	auto bytesToRead = bytesRemaining < bufferSize ? bytesRemaining : bufferSize;
	//
	// 	// Read to buffer
	// 	buffer.resize(bytesToRead / sizeof(double));
	// 	file.read(reinterpret_cast<char*>(buffer.data()), bytesToRead);
	//
	// 	for (size_t i = 0; i < buffer.size(); i += 1) {
	// 		runningStats.push(buffer[i]);
	// 	}
	// 	bytesRemaining -= bytesToRead;
	// }
	//
	// classifyDistribution(runningStats);
	// runningStats.debugPrint();

	// openClTest();

	return 0;

}
