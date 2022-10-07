#pragma once
#include <numeric>
#include <random>
#include <vector>

constexpr auto BLOCK_SIZE = 4 * 1024; // 4 kB
const auto RNG = std::random_device();

class SequenceGenerator {

private:
	int nextIdx = 0;
	std::vector<int> sequence;

public:
	explicit SequenceGenerator(const int limit) {
		this->sequence = std::vector<int>(limit);
		std::iota(std::begin(this->sequence), std::end(this->sequence), 0);
		std::shuffle(std::begin(this->sequence), std::end(this->sequence), std::mt19937{RNG});
	}

	std::vector<int> GetNChunks(const int amount) {
		auto result = std::vector<int>(amount);
		std::copy_n(std::begin(this->sequence) + this->nextIdx, amount, std::begin(result));
		nextIdx += amount;
		return result;
	}
};
