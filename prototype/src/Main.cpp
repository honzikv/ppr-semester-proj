#include <iostream>
#include <random>
#include <cmath>
#include <vector>
#include <fstream>
#include <numeric>


class RunningStats {
public:
    RunningStats();

    void Clear();

    void Push(double x);

    long long NumDataValues() const;

    double Mean() const;

    double Variance() const;

    double StandardDeviation() const;

    double Skewness() const;

    double Kurtosis() const;

    friend RunningStats operator+(const RunningStats a, const RunningStats b);

    RunningStats& operator+=(const RunningStats& rhs);

private:
    long long count;
    double M1, M2, M3, M4;
};


RunningStats::RunningStats() {
    Clear();
}

void RunningStats::Clear() {
    count = 0;
    M1 = M2 = M3 = M4 = 0.0;
}

void RunningStats::Push(double x) {
    double delta, delta_n, delta_n2, term1;

    long long n1 = count;
    count++;
    delta = x - M1;
    delta_n = delta / count;
    delta_n2 = delta_n * delta_n;
    term1 = delta * delta_n * n1;
    M1 += delta_n;
    M4 += term1 * delta_n2 * (count * count - 3 * count + 3) + 6 * delta_n2 * M2 - 4 * delta_n * M3;
    M3 += term1 * delta_n * (count - 2) - 3 * delta_n * M2;
    M2 += term1;
}

long long RunningStats::NumDataValues() const {
    return count;
}

double RunningStats::Mean() const {
    return M1;
}

double RunningStats::Variance() const {
    return M2 / (count - 1.0);
}

double RunningStats::StandardDeviation() const {
    return sqrt(Variance());
}

double RunningStats::Skewness() const {
    return sqrt(double(count)) * M3 / pow(M2, 1.5);
}

double RunningStats::Kurtosis() const {
    return double(count) * M4 / (M2 * M2) - 3.0;
}

RunningStats operator+(const RunningStats stats_a, const RunningStats stats_b) {
    RunningStats combined;

    combined.count = stats_a.count + stats_b.count;

    double delta = stats_b.M1 - stats_a.M1;
    double delta2 = delta * delta;
    double delta3 = delta * delta2;
    double delta4 = delta2 * delta2;

    combined.M1 = (stats_a.count * stats_a.M1 + stats_b.count * stats_b.M1) / combined.count;

    combined.M2 = stats_a.M2 + stats_b.M2 +
                  delta2 * stats_a.count * stats_b.count / combined.count;

    combined.M3 = stats_a.M3 + stats_b.M3 +
                  delta3 * stats_a.count * stats_b.count * (stats_a.count - stats_b.count) / (combined.count * combined.count);
    combined.M3 += 3.0 * delta * (stats_a.count * stats_b.M2 - stats_b.count * stats_a.M2) / combined.count;

    combined.M4 = stats_a.M4 + stats_b.M4 + delta4 * stats_a.count * stats_b.count * (stats_a.count * stats_a.count - stats_a.count * stats_b.count + stats_b.count * stats_b.count) /
                                            (combined.count * combined.count * combined.count);
    combined.M4 += 6.0 * delta2 * (stats_a.count * stats_a.count * stats_b.M2 + stats_b.count * stats_b.count * stats_a.M2) / (combined.count * combined.count) +
                   4.0 * delta * (stats_a.count * stats_b.M3 - stats_b.count * stats_a.M3) / combined.count;

    return combined;
}

RunningStats& RunningStats::operator+=(const RunningStats& rhs) {
    RunningStats combined = *this + rhs;
    *this = combined;
    return *this;
}

#include "structs.h"
int main(int argc, char* argv[]) {

    auto watchDog = WatchDog();

    std::thread t1([&watchDog]() {
        for (int i = 0; i < 1000000; i++) {
            watchDog.IncrementCounter();
            // sleep for 1 second
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

//    auto gauss = std::normal_distribution<double>(0.0, 20000.0);
//    auto rng = std::mt19937_64();
//
//    auto vector = std::vector<double>();
//    for (auto i = 0; i < 4096l * 1024l * 2; i += 1) {
//        vector.push_back(gauss(rng));
//    }
//
//    auto stats = RunningStats();
//    for (auto i = 0; i < vector.size(); i += 1) {
//        stats.Push(vector[i]);
//    }
//
//    std::cout << "Mean: " << stats.Mean() << std::endl;
//    std::cout << "Variance: " << stats.Variance() << std::endl;
//    std::cout << "Standard Deviation: " << stats.StandardDeviation() << std::endl;
//    std::cout << "Skewness: " << stats.Skewness() << std::endl;
//    std::cout << "Kurtosis: " << stats.Kurtosis() << std::endl;
//
//    auto exp = std::exponential_distribution<double>(3.0);
//    auto stats2 = RunningStats();
//
//    for (auto i = 0; i < vector.size(); i += 1) {
//        stats2.Push(exp(rng));
//    }
//
//    std::cout << "Mean: " << stats2.Mean() << std::endl;
//    std::cout << "Variance: " << stats2.Variance() << std::endl;
//    std::cout << "Standard Deviation: " << stats2.StandardDeviation() << std::endl;
//    std::cout << "Skewness: " << stats2.Skewness() << std::endl;
//    std::cout << "Kurtosis: " << stats2.Kurtosis() << std::endl;
//
//    auto file = std::ifstream("uwuntu.iso", std::ios::binary);
//    auto buffer = std::vector<char>(8 * 1024 * 1024);
//
//    auto mean = std::vector<double>{};
//    auto variance = std::vector<double>{};
//    auto skewness = std::vector<double>{};
//    auto kurtosis = std::vector<double>{};
//    while (file.read(buffer.data(), buffer.size())) {
//
//        auto stats = RunningStats();
//        for (auto i = 0; i < buffer.size(); i += 2) {
//            auto value = (buffer[i] << 8) | buffer[i + 1];
//            stats.Push(value);
//        }
//        mean.push_back(stats.Mean());
//        variance.push_back(stats.Variance());
//        skewness.push_back(stats.Skewness());
//        kurtosis.push_back(stats.Kurtosis());
//    }
//
//    // Calculate averages
//    auto mean_avg = std::accumulate(mean.begin(), mean.end(), 0.0) / mean.size();
//    auto variance_avg = std::accumulate(variance.begin(), variance.end(), 0.0) / variance.size();
//    auto skewness_avg = std::accumulate(skewness.begin(), skewness.end(), 0.0) / skewness.size();
//    auto kurtosis_avg = std::accumulate(kurtosis.begin(), kurtosis.end(), 0.0) / kurtosis.size();
//    std::cout << "Mean: " << mean_avg << std::endl;
//    std::cout << "Variance: " << variance_avg << std::endl;
//    std::cout << "Skewness: " << skewness_avg << std::endl;
//    std::cout << "Kurtosis: " << kurtosis_avg << std::endl;
//
//    // Basically we have 2d space - kurtosis and skewness where each distribution is a point
//    auto gauss_point = std::pair<double, double>(0, 0);
//    auto exp_point = std::pair<double, double>(3, 0);
//    auto uniform_point = std::pair<double, double>(0, -6/5);
//
//    // Calculate distance from each point to the average
//    auto gauss_dist = std::sqrt(std::pow(gauss_point.first - mean_avg, 2) + std::pow(gauss_point.second - skewness_avg, 2));
//    auto exp_dist = std::sqrt(std::pow(exp_point.first - mean_avg, 2) + std::pow(exp_point.second - skewness_avg, 2));
//    auto uniform_dist = std::sqrt(std::pow(uniform_point.first - mean_avg, 2) + std::pow(uniform_point.second - skewness_avg, 2));
//
//    // The distribution with the smallest distance is the most likely
//    if (gauss_dist < exp_dist && gauss_dist < uniform_dist) {
//        std::cout << "Gaussian" << std::endl;
//    } else if (exp_dist < gauss_dist && exp_dist < uniform_dist) {
//        std::cout << "Exponential" << std::endl;
//    } else {
//        std::cout << "Uniform" << std::endl;
//    }
//
//    file.close();
}