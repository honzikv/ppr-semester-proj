#include <iostream>
#include <vector>
#include <string>
#include <numeric>


int main() {
    auto vector = std::vector<size_t>(9);
    vector[0] = 1;
//    vector[1] = 4;
//    vector[2] = 2;
//    vector[3] = 3;
//    vector[4] = 5;
//    vector[5] = 6;
//    vector[6] = 1;
//    vector[7] = 2;
//    vector[8] = 123;

    auto sum = std::accumulate(vector.begin(), vector.end(), 0);

    auto itemsToProcess = vector.size();
    while (true) {
        auto nPairs = itemsToProcess / 2 + itemsToProcess % 2;
        for (auto i = 0; i < nPairs; i += 1) {
            vector[i] = i * 2 + 1 < itemsToProcess ? vector[i * 2] + vector[i * 2 + 1] : vector[i * 2];
        }

        if (nPairs == itemsToProcess) {
            break;
        }

        itemsToProcess = nPairs;
    }

    std::cout << vector[0] << std::endl;
    std::cout << "sum: " << sum << std::endl;
}