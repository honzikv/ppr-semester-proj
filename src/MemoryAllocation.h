#pragma once

/**
 * \brief Memory allocation related functionality
 */
namespace MemoryAllocation {

	constexpr auto DEFAULT_APP_MEMORY_LIMIT = 1024 * 1024 * 1024; // 1GB
	constexpr auto DEFAULT_CPU_RUNTIME_RATIO = .3; // 30%
	constexpr auto DEFAULT_CPU_MEMORY_RATIO = .5; // CPU will use 50% of the buffer memory if OpenCL devices are used
	constexpr auto CL_RUNTIME_RATIOS = std::array<double, 4>{.0, .66, .75, .88}; // Indexed by the number of OpenCL devices

	/**
	 * \brief Contains data for memory configuration for all devices
	 */
	struct MemoryConfig {
		/**
		 * \brief Maximum amount of memory that CPU (SMP) can allocate in a job
		 */
		size_t MaxCpuBufferSizeBytes;

		/**
		 * \brief Maximum amount of host memory that can be allocated when copying data
		 *        to CL device for processing
		 */
		size_t MaxClHostBufferSizeBytes;

		/**
		 * \brief Number of CL devices
		 */
		size_t NClDevices;

		/**
		 * \brief Amount of bytes processed by a single StatsAccumulator on CPU
		 */
		size_t BytesPerCpuAccumulator;

		/**
		 * \brief Amount of bytes processed by a single StatsAccumulator on CL device
		 */
		size_t BytesPerClAccumulator;

		MemoryConfig(size_t maxCpuBufferSizeBytes, size_t maxClHostBufferSizeBytes, size_t nClDevices,
		             size_t bytesProcessedPerAccumulatorCpu, size_t bytesProcessedPerAccumulatorCl);
	};

	inline MemoryConfig::MemoryConfig(const size_t maxCpuBufferSizeBytes,
	                                  const size_t maxClHostBufferSizeBytes,
	                                  const size_t nClDevices,
	                                  const size_t bytesProcessedPerAccumulatorCpu,
	                                  const size_t bytesProcessedPerAccumulatorCl):
		MaxCpuBufferSizeBytes(maxCpuBufferSizeBytes),
		MaxClHostBufferSizeBytes(maxClHostBufferSizeBytes),
		NClDevices(nClDevices),
		BytesPerCpuAccumulator(bytesProcessedPerAccumulatorCpu),
		BytesPerClAccumulator(bytesProcessedPerAccumulatorCl) {
	}

	/**
	 * \brief Returns optimal OpenCL memory configuration for specific number of devices.
	 * \param nClDevices number of OpenCL devices that will be used for the computation
	 * \return scaling factor for the memory
	 */
	inline double getClRuntimeRatio(const size_t nClDevices) {
		if (nClDevices >= CL_RUNTIME_RATIOS.size()) {
			// 4 and more devices are not reasonable to use with 1 GB of RAM
			return CL_RUNTIME_RATIOS.back();
		}

		return CL_RUNTIME_RATIOS[nClDevices];
	}

	/**
	 * \brief Builds memory config for the application
	 * \param processingConfig processing config
	 * \param appMemoryLimit total memory limit for the application - e.g. 1 GB
	 * \param bytesProcessedPerAccumulatorCpu amount of bytes processed by a single StatsAccumulator on CPU
	 * \param bytesProcessedPerAccumulatorCl amount of bytes processed by a single StatsAccumulator on CL device
	 * \return configured instance of MemoryConfig
	 */
	inline MemoryConfig buildMemoryConfig(const ProcessingConfig& processingConfig,
	                                      const size_t bytesProcessedPerAccumulatorCpu,
	                                      const size_t bytesProcessedPerAccumulatorCl,
	                                      const size_t appMemoryLimit = DEFAULT_APP_MEMORY_LIMIT

	) {
		const auto processingMode = processingConfig.ProcessingMode;
		const auto clRuntimeRatio = getClRuntimeRatio(processingConfig.ClDevices.size());

		const auto appRuntimeRatio = (processingMode == ProcessingMode::OPENCL_DEVICES || processingMode ==
			                             ProcessingMode::ALL)
			                             ? clRuntimeRatio
			                             : DEFAULT_CPU_RUNTIME_RATIO;

		// Max amount of memory for all buffers (both CPU and CL)
		const auto bufferMemoryLimit = static_cast<size_t>(static_cast<double>(appMemoryLimit) * (1 - appRuntimeRatio));

		// If we are using SMP or single threaded we only allocate memory to the CPU
		if (processingMode == ProcessingMode::SMP || processingMode == ProcessingMode::SINGLE_THREAD) {
			return {
				bufferMemoryLimit,
				0,
				0,
				bytesProcessedPerAccumulatorCpu,
				0,
			};
		}

		// Otherwise our mode is either OpenCL devices or ALL
		size_t maxCpuBufferBytes = 0;
		size_t clDevicesMemory;
		const auto nClDevices = processingConfig.ClDevices.size();
		if (processingMode == ProcessingMode::ALL) {
			// There is a special case when we use ALL and do not actually have any CL device.
			// This would use only % of the available memory and slow down the computation.
			// Therefore check if there is at least one CL device available and if not just use all the memory for CPU
			if (nClDevices == 0) {
				return {
					bufferMemoryLimit,
					0,
					0,
					bytesProcessedPerAccumulatorCpu,
					0,
				};
			}

			maxCpuBufferBytes = static_cast<size_t>(static_cast<double>(bufferMemoryLimit) * DEFAULT_CPU_MEMORY_RATIO);
			clDevicesMemory = bufferMemoryLimit - maxCpuBufferBytes;
		}
		else {
			clDevicesMemory = bufferMemoryLimit;
		}

		// Calculate how much memory we can allocate for each CL device
		const auto maxClHostBufferSizeBytes = clDevicesMemory / nClDevices / 2;

		return {
			maxCpuBufferBytes,
			maxClHostBufferSizeBytes,
			nClDevices,
			bytesProcessedPerAccumulatorCpu,
			bytesProcessedPerAccumulatorCl,
		};
	}
}
