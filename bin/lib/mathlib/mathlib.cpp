#define _USE_MATH_DEFINES
#include "mathlib.hpp"
#include <string>
#include <cmath>

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
bool __Math__::InRange(long double value, long double min, long double max) {
	return value >= min && value <= max;
}
bool __Math__::OutRange(long double value, long double min, long double max) {
	return value < min || value > max;
}

std::string __Math__::fracstr(std::pair<int, int> fvar) {
	return std::to_string(fvar.first) + " / " + std::to_string(fvar.second);
}

int __Math__::frac_fir(std::pair<int, int> fvar) {
	return fvar.first;
}
int __Math__::frac_sec(std::pair<int, int> fvar) {
	return fvar.second;
}
double __Math__::PI() {
	return M_PI;
}
int __Math__::sin(int v) {
	return std::sin(v);
}
int __Math__::cos(int v) {
	return std::cos(v);
}
int __Math__::tan(int v) {
	return std::tan(v);
}