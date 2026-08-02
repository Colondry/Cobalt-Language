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
long double rand_stable(long double min, long double max);

#endif // !BASEUTILS
