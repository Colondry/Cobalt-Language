#include "base.hpp"
#include <ctime>
#include <cmath>
#include <cctype>
#include <cstring>
#include <string>
#include <random>
#include "../mathlib/mathlib.hpp"

int __str__::len(std::string var) {
	return var.length();
}
bool __str__::startsWith(const std::string& word, const std::string& start) {
	if (word.length() < start.length()) {
		return false;
	}
	// Compare starting at index 0, for prefix.length() characters
	return word.compare(0, start.length(), start) == 0;
}
bool __str__::endWith(const std::string& word, const std::string& end) {
	if (end.length() > word.length()) return false;
	return word.rfind(end) == (word.length() - end.length());
}
std::string __str__::upper(std::string word) {
	for (char &c : word) {
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}
	return word;
}
std::string __str__::lower(std::string word) {
	for (char &c : word) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return word;
}


clock_t __Time__::Now() {
	return std::clock();
}
long double __Time__::Elapsed(clock_t start, clock_t end) {
	return static_cast<long double>(end - start) / CLOCKS_PER_SEC;
}

long double rand(long double min, long double max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> distrib(min, max);
	return distrib(gen);
}
long double rand_stable(long double min, long double max)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	static long double previous = (min + max) / 2;

	std::uniform_real_distribution<long double> dist(min, max);

	long double target = dist(gen);

	previous = previous * 0.8 + target * 0.2;

	return previous;
}
