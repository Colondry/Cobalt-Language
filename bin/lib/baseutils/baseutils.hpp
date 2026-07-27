#ifndef BASEUTILS
#define BASEUTILS

#include <string>
#include <ctime>

class __str__ {
public:
	int len(std::string var);
	bool startsWith(const std::string& word, const std::string& start);
	bool endWith(const std::string& word, const std::string& end);
	std::string upper(std::string word);
	std::string lower(std::string word);
};

class __Math__ {
public:
	long double power(long double x, long double y);
	double sqroot(double y);
	double absolute(double y);
	int minimum(int x, int y);
	int maximum(int x, int y);
	long double clamp(long double value, long double min, long double max);
	int clint(int value, int min, int max);
};

class __File__ {
public:
	void outf(const std::string name);
	void delf(const std::string name);
};

class __Time__ {
public:
	clock_t Now();
	long double Elapsed(clock_t start, clock_t end);
};

long double rand(long double min, long double max);

#endif // !BASEUTILS
