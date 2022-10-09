
#include <fstream>
#include <iostream>
#include <CL/cl.hpp>

const auto HELLO_WORLD_CL = "openClTest.cl";

inline auto LoadClFiles() {
	auto helloWorldFile = std::ifstream(HELLO_WORLD_CL);
	auto src = std::string(std::istreambuf_iterator<char>(helloWorldFile), (std::istreambuf_iterator<char>()));

	auto sources = cl::Program::Sources(1, { src.c_str(), src.length() + 1 });

	return sources;
}

inline auto GetGpuDevice() {
	auto platforms = std::vector<cl::Platform>();
	cl::Platform::get(&platforms);

	const auto platform = platforms.front();
	auto devices = std::vector<cl::Device>();

	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	auto device = devices.front();
	return device;
}

inline auto TestOpenCl() {
	const auto device = GetGpuDevice();
	const auto ctx = cl::Context(device);
	const auto sources = LoadClFiles();
	const auto program = cl::Program(ctx, sources);
	program.build();

	auto buffer = std::vector<char>(16);
	auto memoryBuffer = cl::Buffer(cl::Context(device), CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, 16 * sizeof(int));
	auto kernel = cl::Kernel(program, "HelloWorld");
	kernel.setArg(0, memoryBuffer);

	auto commandQueue = cl::CommandQueue(ctx, device);
	commandQueue.enqueueNDRangeKernel(kernel, NULL, 1, 1);
	commandQueue.enqueueReadBuffer(memoryBuffer, CL_TRUE, 0, 16 * sizeof(int), buffer.data());
	std::cout << buffer.data();
	std::cin.get();
}

