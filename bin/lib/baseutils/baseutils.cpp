#include "baseutils.hpp"
#include <ctime>
#include <cmath>
#include <cctype>
#include <cstring>
#include <string>
#include <random>

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

long double __Math__::power(long double x, long double y) {
	return pow(x, y);
}
double __Math__::sqroot(double y) {
	return sqrt(y);
}
double __Math__::absolute(double y) {
	return std::abs(y);
}
int __Math__::minimum(int x, int y) {
	return std::min(x, y);
}
int __Math__::maximum(int x, int y) {
	return std::max(x, y);
}
long double __Math__::clamp(long double value, long double min, long double max) {
	return (value < min) ? min : ((value > max) ? max : value);
}
int __Math__::clint(int value, int min, int max) {
	int v = value ^ ((value ^ min) & -(value < min));
	return v ^ ((v ^ max) & -(v > max));
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
