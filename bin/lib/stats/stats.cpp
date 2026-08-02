#include "stats.hpp"
#include  <vector>
#include <cmath>
#include <algorithm>
#include <random>

double __Stats__::Mean(const std::vector<double>& data)
{
    double sum = 0;

    for (double x : data)
        sum += x;

    return sum / data.size();
}

double __Stats__::Sum(const std::vector<double>& data)
{
    double sum = 0;

    for (double x : data)
        sum += x;

    return sum;
}

double __Stats__::Max(const std::vector<double>& data)
{
    double m = data[0];

    for (double x : data)
        if (x > m)
            m = x;

    return m;
}

double __Stats__::Min(const std::vector<double>& data)
{
    double m = data[0];

    for (double x : data)
        if (x < m)
            m = x;

    return m;
}

double __Stats__::Median(std::vector<double> data)
{
    std::sort(data.begin(), data.end());

    size_t n = data.size();

    if (n % 2 == 0)
        return (data[n / 2 - 1] + data[n / 2]) / 2;

    return data[n / 2];
}
double __Stats__::Variance(
    const std::vector<double>& data)
{
    double mean = Mean(data);

    double sum = 0;

    for (double x : data)
    {
        double d = x - mean;
        sum += d * d;
    }

    return sum / data.size();
}
double __Stats__::StdDev(const std::vector<double>& data) {
    return std::sqrt(Variance(data));
}
void __Stats__::Shuffle(std::vector<double>& data) {
    std::random_device rd;

    std::mt19937 gen(rd());
    std::shuffle(
        data.begin(),
        data.end(),
        gen
    );
}
