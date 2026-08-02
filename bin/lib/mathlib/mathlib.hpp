#ifndef MATH_LIB
#define MATH_LIB

#include <string>
#include <utility>

class __Math__ {
public:
	long double power(long double x, long double y);
	double sqroot(double y);
	double absolute(double y);
	int minimum(int x, int y);
	int maximum(int x, int y);
	long double clamp(long double value, long double min, long double max);
	int clint(int value, int min, int max);
	bool InRange(long double value, long double min, long double max);
	bool OutRange(long double value, long double min, long double max);
	long double Reflect(long double value);
	template<typename T>
	T Reflect(T value) {
		return -value;
	}

	std::string fracstr(std::pair<int, int> fvar);
	int frac_fir(std::pair<int, int> fvar);
	int frac_sec(std::pair<int, int> fvar);
	double PI();
	int sin(int v);
	int cos(int v);
	int tan(int v);
};

#endif