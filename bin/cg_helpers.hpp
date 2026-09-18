#ifndef CODEGEN_HELPERS_HPP
#define CODEGEN_HELPERS_HPP

#include <string>
#include <cstdint>

// Type mapping from Cobalt's types to C++ types.
inline std::string cppType(const std::string& t, const std::string& context = "") {
    (void)context;
    if (t == "string") return "std::string";
    if (t == "byte") return "__byte__";
    if (t == "std::uint8_t") return "__byte__";
    return t;
}

inline std::string indent(int depth) {
    return std::string(depth * 4, ' ');
}

// True if `s` is a plain non-negative or negative integer literal.
inline bool isNumericLiteral(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i)
        if (s[i] < '0' || s[i] > '9') return false;
    return true;
}

#endif