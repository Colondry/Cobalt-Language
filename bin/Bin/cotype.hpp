#ifndef COBALT_CAST_HPP
#define COBALT_CAST_HPP

#include <string>

namespace csm {
    inline int toInt(const std::string& s) { return std::stoi(s); }
    inline double toDouble(const std::string& s) { return std::stod(s); }
    inline long long toLong(const std::string& s) { return std::stoll(s); }
    inline long double toLonger(const std::string& s) { return std::stold(s); }
    inline std::string toStr(const int v) { return std::to_string(v); }
    inline std::string toStr(const double v) { return std::to_string(v); }
    inline std::string toStr(const long long v) { return std::to_string(v); }
    inline std::string toStr(const long double v) { return std::to_string(v); }
    inline std::string toStr(const char v) { return std::string(1, v); }
    inline std::string toStr(const std::string& v) { return v; }
    inline char toChar(const std::string& v) { return v.empty() ? '\0' : v[0]; }
    inline char toChar(const int v) { return static_cast<char>(v); }
    inline char toChar(const double v) { return static_cast<char>(v); }
    inline char toChar(const long long v) { return static_cast<char>(v); }
    inline char toChar(const long double v) { return static_cast<char>(v); }
}

#endif