#pragma once
#include <numeric>

#include "Arghandling.h"

namespace MemoryUtils {
	constexpr auto DEFAULT_MEMORY_SIZE = 768 * 1024 * 1024; // 1 GB for everything

	/**
	 * \brief 20% of the memory is kept for structures (i.e. discarding coordinator buffers) such as
	 *		  JobScheduler, Running stats, and so on. Basically anything that is not RAM memory buffer
	 */
	constexpr auto DEFAULT_RUNTIME_STRUCTS_MEMORY_RATIO = .2;

	/**
	 * \brief If CPU uses 60% of the memory, rest is split between copy buffers for CL devices
	 */
	constexpr auto DEFAULT_CPU_MEMORY_RATIO = .6;

	/**
	 * \brief Simple struct containing amount of memory for each device
	 */
	struct CoordinatorMemoryLimits {
		size_t CpuMemory; // Max amount of memory for CPU

		/**
		 * \brief Max amount of memory for a single CL device - this can be actually quite small since
		 * we only use this memory to copy the data into device buffer
		 */
		size_t ClDeviceMemory;
		size_t ClDeviceCount; // Number of CL devices

		CoordinatorMemoryLimits(const size_t cpuMemory, const size_t clDeviceMemory, const size_t clDeviceCount):
			CpuMemory(cpuMemory),
			ClDeviceMemory(clDeviceMemory),
			ClDeviceCount(clDeviceCount) {
		}
	};

	/**
	 * \brief Computes memory limits for CPU and CL coordinators
	 * \param processingInfo processing info
	 * \param appMemoryLimit memory limit for the entire application
	 * \param cpuMemoryRatio % of buffer memory used by the CPU
	 * \param runtimeStructsSize % size of runtime structures
	 * \return instance of CoordinatorMemoryLimits
	 */
	inline auto computeCoordinatorMemoryLimits(const ProcessingInfo& processingInfo,
	                                           const size_t appMemoryLimit = DEFAULT_MEMORY_SIZE,
	                                           const double cpuMemoryRatio = DEFAULT_CPU_MEMORY_RATIO,
	                                           const double runtimeStructsSize = DEFAULT_RUNTIME_STRUCTS_MEMORY_RATIO) {
		// Compute memory for buffers
		const auto bufferMemory = static_cast<size_t>(static_cast<double>(appMemoryLimit) * (1 - runtimeStructsSize));

		if (processingInfo.ProcessingMode == ProcessingMode::SMP || processingInfo.ProcessingMode ==
			ProcessingMode::SINGLE_THREAD) {
			// If we are in SMP / Single thread mode we simply use all buffer memory
			return CoordinatorMemoryLimits(bufferMemory, 0, 0);
		}

		// Otherwise split the memory
		size_t cpuMemory = 0;
		size_t clDevicesMemory;

		// Check how many CL devices are available
		const auto nClDevices = std::accumulate(processingInfo.Devices.begin(), processingInfo.Devices.end(), 0,
		                                        [&](auto sum, auto item) {
			                                        auto [_, devices] = item;
			                                        return sum + devices.size();
		                                        }
		);

		if (processingInfo.ProcessingMode == ProcessingMode::ALL) {
			// There is a special case when we use ALL and do not actually have any CL device.
			// This would use only % of the available memory and slow down the computation.
			// Therefore check if there is at least one CL device available and if not just use all the memory for CPU
			if (nClDevices == 0) {
				return CoordinatorMemoryLimits(bufferMemory, 0, 0);
			}

			cpuMemory = static_cast<size_t>(static_cast<double>(bufferMemory) * cpuMemoryRatio);
			clDevicesMemory = bufferMemory - cpuMemory;
		}
		else {
			clDevicesMemory = bufferMemory;
		}

		return CoordinatorMemoryLimits(cpuMemory, clDevicesMemory, nClDevices);
	}
}
