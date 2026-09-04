#ifndef BASEUTILS
#define BASEUTILS

#include <string>
#include <ctime>
#include <memory>


class str {
public:
	int len(std::string var);
	bool startsWith(const std::string& word, const std::string& start);
	bool endWith(const std::string& word, const std::string& end);
	std::string upper(std::string word);
	std::string lower(std::string word);
};

class File {
public:
	void outf(const std::string name);
	void delf(const std::string name);
};

class Time {
public:
	clock_t Now();
	
	long double Elapsed(clock_t start, clock_t end) {
		return static_cast<long double>(end - start) / CLOCKS_PER_SEC;
	}
	inline long double Elapsed(const std::unique_ptr<clock_t>& start, const std::unique_ptr<clock_t>& end) {
		return Elapsed(start ? *start : 0, end ? *end : 0);
	}

	inline long double Elapsed(const std::unique_ptr<clock_t>& start, clock_t end) {
		return Elapsed(start ? *start : 0, end);
	}

	inline long double Elapsed(clock_t start, const std::unique_ptr<clock_t>& end) {
		return Elapsed(start, end ? *end : 0);
	}
};

long double rand(long double min, long double max);
long double rand_stable(long double min, long double max);

#endif // !BASEUTILS
