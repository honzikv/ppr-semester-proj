#include <oneapi/tbb.h>
#include "Arghandling.h"
#include "DistributionClassification.h"
#include "JobScheduler.h"
#include <fstream>
#include "SingleThreadStatsComputation.h"

#include "OpenClTest.h"
#include "StatUtils.h"

int main(int argc, char** argv) {
	auto args = parseArguments(argc, argv);
	auto processingInfo = validateArguments(args);
	auto jobScheduler = JobScheduler(processingInfo);
	auto res = jobScheduler.run();

	std::cout << sizeof(StatsAccumulator) * res.size() << std::endl;
	// Classify the result
	classifyDistribution(StatUtils::mergeLeftToRight(res));

	// std::cout << "\nTEST SINGLE THREAD" << std::endl;
	// auto filePath = processingInfo.DistFilePath;
	//
	// auto statsAccumulators = std::vector<StatsAccumulator>();
	//
	// auto buffer = std::vector<double>();
	// auto file = std::ifstream(filePath, std::ios::binary);
	//
	// auto bufferSize = 8 * 1024 * 1024;
	// auto fileSize = fs::file_size(filePath);
	// auto nReads = static_cast<size_t>(ceil(fileSize / bufferSize));
	// statsAccumulators.resize(nReads);
	//
	// auto bytesRemaining = fileSize;
	// for (auto bufferId = 0; bufferId < nReads; bufferId += 1) {
	// 	auto bytesToRead = bytesRemaining < bufferSize ? bytesRemaining : bufferSize;
	//
	// 	// Read to buffer
	// 	buffer.resize(bytesToRead / sizeof(double));
	// 	file.read(reinterpret_cast<char*>(buffer.data()), bytesToRead);
	//
	// 	for (size_t i = 0; i < buffer.size(); i += 1) {
	// 		statsAccumulators[bufferId].push(buffer[i]);
	// 	}
	//
	// 	bytesRemaining -= bytesToRead;
	// }
	//
	// std::cout << "Merged from left to right" << std::endl;
	// auto resultMergedLeftRight = statsAccumulators[0];
	// for (auto i = 1ULL; i < statsAccumulators.size(); i += 1) {
	// 	resultMergedLeftRight += statsAccumulators[i];
	// }
	// classifyDistribution(resultMergedLeftRight);
	//
	//
	// std::cout << "Reduced from right to left" << std::endl;
	// auto itemsToProcess = statsAccumulators.size();
	// while (true) {
	// 	auto nPairs = itemsToProcess / 2 + itemsToProcess % 2;
	// 	for (auto i = 0ULL; i < nPairs; i += 1) {
	// 		statsAccumulators[i] = i * 2 + 1 < itemsToProcess ? statsAccumulators[i * 2] + statsAccumulators[i * 2 + 1] : statsAccumulators[i * 2];
	// 	}
	//
	// 	if (nPairs == itemsToProcess) {
	// 		break;
	// 	}
	//
	// 	itemsToProcess = nPairs;
	// }
	// classifyDistribution(statsAccumulators[0]);


	// std::cout << "\nSingle Thread Test\n" << std::endl;
	// const auto singleThreadComputation = SingleThreadStatsComputation(4096, args, 8, 256 * 1024 * 1024 / 6);
	//
	// auto results = singleThreadComputation.run();
	// std::cout << "N results: " << results.size() << std::endl;
	// classifyDistribution(StatUtils::mergePairwise(results));


	// openClTest();

	return 0;

}
