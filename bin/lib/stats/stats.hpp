#ifndef STATS
#define STATS

#include <vector>

class __Stats__ {
public:
	double Mean(const std::vector<double>& data);
	double Sum(const std::vector<double>& data);
	double Max(const std::vector<double>& data);
	double Min(const std::vector<double>& data);
	double Median(std::vector<double> data);
	double Variance(
		const std::vector<double>& data);
	double StdDev(const std::vector<double>& data);
	void Shuffle(std::vector<double>& data);
};

#endif