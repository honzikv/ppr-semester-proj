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


}