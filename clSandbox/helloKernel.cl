__kernel void processArray(__global double* array, __global double* bufferSize) {
    size_t threadIdx = get_global_id(0);
    size_t nItemsToProcess = *bufferSize / threadIdx;

    for (size_t i = 0; i < nItemsToProcess; i += 1) {
        array[i + 1] = (double)threadIdx;
    }
}
