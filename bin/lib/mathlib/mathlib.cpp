#define _USE_MATH_DEFINES
#include "mathlib.hpp"
#include <string>
#include <cmath>

long double Math::power(long double x, long double y) {
	return pow(x, y);
}
double Math::sqroot(double y) {
	return sqrt(y);
}
double Math::absolute(double y) {
	return std::abs(y);
}
int Math::minimum(int x, int y) {
	return std::min(x, y);
}
int Math::maximum(int x, int y) {
	return std::max(x, y);
}
long double Math::clamp(long double value, long double min, long double max) {
	return (value < min) ? min : ((value > max) ? max : value);
}
int Math::clint(int value, int min, int max) {
	int v = value ^ ((value ^ min) & -(value < min));
	return v ^ ((v ^ max) & -(v > max));
}
bool Math::InRange(long double value, long double min, long double max) {
	return value >= min && value <= max;
}
bool Math::OutRange(long double value, long double min, long double max) {
	return value < min || value > max;
}

std::string Math::fracstr(std::pair<int, int> fvar) {
	return std::to_string(fvar.first) + " / " + std::to_string(fvar.second);
}

int Math::frac_fir(std::pair<int, int> fvar) {
	return fvar.first;
}
int Math::frac_sec(std::pair<int, int> fvar) {
	return fvar.second;
}
double Math::PI() {
	return M_PI;
}
int Math::sin(int v) {
	return std::sin(v);
}
int Math::cos(int v) {
	return std::cos(v);
}
int Math::tan(int v) {
	return std::tan(v);
}