#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <type_traits>
#include <limits>
#include <utility>
#include <cstdint>
#include <format>
#include <string_view>
#include <stdfloat>
#include <string>

#if defined(__STDCPP_FLOAT128_T__)
inline std::ostream& operator<<(std::ostream& os, std::float128_t v) {
    return os << static_cast<long double>(v);
}
#endif

template<typename __L__, typename __R__>
auto __cobalt_add__(const __L__& l, const __R__& r) {
    if constexpr (std::is_convertible_v<std::decay_t<__L__>, std::string> ||
                  std::is_convertible_v<std::decay_t<__R__>, std::string>) [[unlikely]] {
        std::ostringstream __oss__;
        __oss__ << l << r;
        return __oss__.str();
    } else [[likely]] {
        return l + r;
    }
}

inline void syncw_stdio(bool s) {
    std::ios_base::sync_with_stdio(s);
}

template<typename T>
inline void __cobalt_readln__(const std::string& prompt, T& target, char delim = '\n') {
    std::cout << prompt;

    if constexpr (std::is_same_v<T, std::string>) [[unlikely]] {
        std::getline(std::cin, target, delim);
    } else [[likely]] {
        std::cin >> target;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), delim);
    }
}

template<typename T>
inline void __cobalt_readln__(const char* prompt, T& target, char delim = '\n') {
    std::cout << prompt;

    if constexpr (std::is_same_v<T, std::string>) [[unlikely]] {
        std::getline(std::cin, target, delim);
    } else [[likely]] {
        std::cin >> target;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), delim);
    }
}

template<typename __FracA__, typename __FracB__>
struct __Fraction__ {
    __FracA__ first;
    __FracB__ second;
    operator std::pair<__FracA__, __FracB__>() const { return {first, second}; }
};

template<typename __FracA__, typename __FracB__>
inline std::ostream& operator<<(std::ostream& os, const __Fraction__<__FracA__, __FracB__>& f) {
    return os << f.first << "/" << f.second;
}

struct __cobalt_byte__ {
    std::uint8_t v = 0;
    __cobalt_byte__() = default;
    __cobalt_byte__(long long x) : v(static_cast<std::uint8_t>(x)) {}
    operator int() const { return v; }
};

inline std::ostream& operator<<(std::ostream& os, __cobalt_byte__ b) {
    return os << static_cast<int>(b.v);
}

inline std::istream& operator>>(std::istream& is, __cobalt_byte__& b) {
    int __tmp__; 
    is >> __tmp__; 
    b.v = static_cast<std::uint8_t>(__tmp__); 
    return is;
}

template<typename... Args>
inline void cprintln(std::string_view fmt, Args&&... args) {
    std::cout << std::vformat(fmt, std::make_format_args(args...)) << '\n';
}

template<typename... Args>
inline void cprint(std::string_view fmt, Args&&... args) {
    std::cout << std::vformat(fmt, std::make_format_args(args...));
}
#include <cstring>
#include <cstddef>

struct c_string {
    static constexpr size_t MAX_SIZE = 2048;
    char data[MAX_SIZE] = "";
    size_t length = 0;

    // 1. Default Constructor: c_string s;
    c_string() : length(0) { data[0] = '\0'; }

    // 2. Implicit Literal Constructor: c_string name = "Hello";
    c_string(const char* str) {
        assign(str);
    }

    // 3. Copy Constructor
    c_string(const c_string& other) {
        assign(other.data);
    }

    // 4. Assignment Operator: name = "World";
    c_string& operator=(const char* str) {
        assign(str);
        return *this;
    }

    // 5. Assignment Operator between CobaltStrings
    c_string& operator=(const c_string& other) {
        if (this != &other) {
            assign(other.data);
        }
        return *this;
    }

    // 6. Concatenation Operator: s1 + s2 or s1 + "lit"
    c_string operator+(const c_string& other) const {
        c_string result = *this;
        if (result.length + other.length < MAX_SIZE) {
            std::memcpy(result.data + result.length, other.data, other.length + 1);
            result.length += other.length;
        }
        return result;
    }

    // 7. Implicit Conversion to `const char*` (Allows C-style functions & easy passing)
    operator const char*() const { return data; }

    // 8. Array Indexing: s[0]
    char& operator[](size_t index) { return data[index]; }
    const char& operator[](size_t index) const { return data[index]; }

    // Comparison Operators
    bool operator==(const c_string& other) const {
        return std::strcmp(data, other.data) == 0;
    }
    bool operator==(const char* str) const {
        return std::strcmp(data, str) == 0;
    }

private:
    void assign(const char* str) {
        if (!str) {
            data[0] = '\0';
            length = 0;
            return;
        }
        length = std::strlen(str);
        if (length >= MAX_SIZE) length = MAX_SIZE - 1;
        std::memcpy(data, str, length);
        data[length] = '\0';
    }
};

// 9. Enable std::cout << name; directly!
inline std::ostream& operator<<(std::ostream& os, const c_string& str) {
    return os << str.data;
}