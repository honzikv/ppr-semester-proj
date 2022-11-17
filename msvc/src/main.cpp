#include <iostream>
#include <oneapi/tbb.h>
#include "Arghandling.h"
#include "JobScheduler.h"
#include <oneapi/tbb.h>

int main(int argc, char** argv) {
	auto args = parseArguments(argc, argv);
	auto processingInfo = validateArguments(args);

    auto values = std::vector<double>(10000);

    // Initialize the values with random values starting from 0 to 1
	std::generate(values.begin(), values.end(), []() { return static_cast<double>(rand()) / RAND_MAX; });

    auto sums = std::vector<double>(8);

    std::mutex m;
    oneapi::tbb::parallel_for(tbb::blocked_range<int>(0, values.size()),
        [&](tbb::blocked_range<int> r)
        {   
            for (int i = r.begin(); i < r.end(); ++i)
            {
				sums[oneapi::tbb::this_task_arena::current_thread_index()] += values[i];
            }
        });

	for (auto i = 0; i < sums.size(); ++i)
	{
		std::cout << "Sum " << i << " = " << sums[i] << std::endl;
	}

    return 0;
	
}
