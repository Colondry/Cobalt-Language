#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <type_traits>
#include <numeric>
#include <limits>
#include <utility>
#include <cstdint>
#include <format>
#include <string_view>
#include <stdfloat>
#include <string>
#include <cstddef>

#if defined(__STDCPP_FLOAT128_T__)
inline std::ostream& operator<<(std::ostream& os, std::float128_t v) {
    return os << static_cast<long double>(v);
}
#endif

template<typename __L__, typename __R__>
auto __cadd__(const __L__& l, const __R__& r) {
    using DecayedL = std::decay_t<__L__>;
    using DecayedR = std::decay_t<__R__>;

    // Detect if either operand is string-like (std::string, const char*, c_string)
    constexpr bool is_string_op = 
        std::is_convertible_v<DecayedL, std::string> ||
        std::is_convertible_v<DecayedR, std::string>;

    if constexpr (is_string_op) [[unlikely]] {
        std::ostringstream __oss__;
        __oss__ << l << r; // Uses frac's operator<< automatically
        return __oss__.str();
    } else [[likely]] {
        return l + r; // Invokes frac::operator+ or scalar addition
    }
}
template<typename __L__>
auto __cadd__(const __L__& l, const __L__& r) {
    return __cadd__<__L__, __L__>(l, r);
}

template<typename T>
inline void readln(const std::string& prompt, T& target, char delim = '\n') {
    std::cout << prompt;

    if constexpr (std::is_same_v<T, std::string>) [[unlikely]] {
        std::getline(std::cin, target, delim);
    } else [[likely]] {
        std::cin >> target;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), delim);
    }
}

template<typename T>
inline void readln(const char* prompt, T& target, char delim = '\n') {
    std::cout << prompt;

    if constexpr (std::is_same_v<T, std::string>) [[unlikely]] {
        std::getline(std::cin, target, delim);
    } else [[likely]] {
        std::cin >> target;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), delim);
    }
}

// Forward declaration
template<typename T1, typename T2 = T1>
struct frac;

// Standalone free function declarations
template<typename T1, typename T2>
constexpr frac<T1, T2>& simplify(frac<T1, T2>& f);

template<typename T1, typename T2>
[[nodiscard]] constexpr frac<T1, T2> simplified(const frac<T1, T2>& f);

template<typename T1, typename T2>
struct frac {
    T1 first{};
    T2 second{1}; // Prevent division by zero default

    // --- Constructors ---
    constexpr frac() = default;
    constexpr frac(T1 f) : first(std::move(f)), second(1) {}
    constexpr frac(T1 f, T2 s) : first(std::move(f)), second(std::move(s)) {
        normalize_sign();
        simplify(*this);
    }

    // --- Explicit Conversions ---
    constexpr operator std::pair<T1, T2>() const {
        return {first, second};
    }

    explicit constexpr operator double() const {
        return static_cast<double>(first) / static_cast<double>(second);
    }

    explicit constexpr operator float() const {
        return static_cast<float>(first) / static_cast<float>(second);
    }

    // --- Unary Operators ---
    constexpr frac operator-() const {
        return frac(-first, second);
    }

    // --- Utilities ---
    [[nodiscard]] constexpr frac reciprocal() const {
        return frac(second, first);
    }

    [[nodiscard]] constexpr frac abs() const {
        return frac(first < 0 ? -first : first, second);
    }

    [[nodiscard]] constexpr frac pow(int exp) const {
        if (exp == 0) return frac(1, 1);
        if (exp < 0) return reciprocal().pow(-exp);
        frac result(1, 1);
        frac base = *this;
        while (exp > 0) {
            if (exp % 2 == 1) result *= base;
            base *= base;
            exp /= 2;
        }
        return result;
    }

    // --- Compound Arithmetic Operators ---
    constexpr frac& operator+=(const frac& other) {
        first = (first * other.second) + (other.first * second);
        second = second * other.second;
        return simplify(*this);
    }

    constexpr frac& operator-=(const frac& other) {
        first = (first * other.second) - (other.first * second);
        second = second * other.second;
        return simplify(*this);
    }

    constexpr frac& operator*=(const frac& other) {
        // Overflow-safe cross-simplification before multiplication
        frac f1 = simplified(frac(first, other.second));
        frac f2 = simplified(frac(other.first, second));
        first = f1.first * f2.first;
        second = f1.second * f2.second;
        return simplify(*this);
    }

    constexpr frac& operator/=(const frac& other) {
        return *this *= other.reciprocal();
    }

    // --- Binary Member Operators ---
    constexpr frac operator+(const frac& other) const { return frac(*this) += other; }
    constexpr frac operator-(const frac& other) const { return frac(*this) -= other; }
    constexpr frac operator*(const frac& other) const { return frac(*this) *= other; }
    constexpr frac operator/(const frac& other) const { return frac(*this) /= other; }

    // --- Mixed Scalar Arithmetic ---
    template<typename U> constexpr auto operator+(U scalar) const { return *this + frac(scalar); }
    template<typename U> constexpr auto operator-(U scalar) const { return *this - frac(scalar); }
    template<typename U> constexpr auto operator*(U scalar) const { return *this * frac(scalar); }
    template<typename U> constexpr auto operator/(U scalar) const { return *this / frac(scalar); }

    // --- C++20 Comparisons ---
    constexpr auto operator<=>(const frac& other) const {
        auto lhs = first * other.second;
        auto rhs = other.first * second;
        return lhs <=> rhs;
    }
    constexpr bool operator==(const frac& other) const {
        return (first * other.second) == (other.first * second);
    }

private:
    constexpr void normalize_sign() {
        if constexpr (std::is_signed_v<T2>) {
            if (second < 0) {
                first = -first;
                second = -second;
            }
        }
    }
};

// --- Standalone Simplify Functions ---
template<typename T1, typename T2>
constexpr frac<T1, T2>& simplify(frac<T1, T2>& f) {
    if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
        if (f.second == 0) return f;
        auto g = std::gcd(f.first, f.second);
        if (g != 0) {
            f.first /= g;
            f.second /= g;
        }
        if constexpr (std::is_signed_v<T2>) {
            if (f.second < 0) {
                f.first = -f.first;
                f.second = -f.second;
            }
        }
    }
    return f;
}

template<typename T1, typename T2>
[[nodiscard]] constexpr frac<T1, T2> simplified(const frac<T1, T2>& f) {
    frac<T1, T2> result = f;
    return simplify(result);
}

// --- Global Symmetric Operators for Scalar Left-Hand Operations ---
template<typename U, typename T1, typename T2>
constexpr auto operator+(U scalar, const frac<T1, T2>& f) { return frac<T1, T2>(scalar) + f; }

template<typename U, typename T1, typename T2>
constexpr auto operator-(U scalar, const frac<T1, T2>& f) { return frac<T1, T2>(scalar) - f; }

template<typename U, typename T1, typename T2>
constexpr auto operator*(U scalar, const frac<T1, T2>& f) { return frac<T1, T2>(scalar) * f; }

template<typename U, typename T1, typename T2>
constexpr auto operator/(U scalar, const frac<T1, T2>& f) { return frac<T1, T2>(scalar) / f; }

// --- CTAD Guide ---
template<typename T1, typename T2>
frac(T1, T2) -> frac<T1, T2>;

// --- User-Defined Literals ---
namespace literals {
    constexpr frac<long long> operator""_frac(unsigned long long n) {
        return frac<long long>(static_cast<long long>(n), 1LL);
    }
}

// --- Formatting & I/O ---
template<typename T1, typename T2>
inline std::ostream& operator<<(std::ostream& os, const frac<T1, T2>& f) {
    return os << f.first << '/' << f.second;
}

template<typename T1, typename T2>
struct std::formatter<frac<T1, T2>> : std::formatter<std::string_view> {
    auto format(const frac<T1, T2>& f, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}/{}", f.first, f.second);
    }
};

struct __cobalt_byte__ {
    __cobalt_byte__() = default;
    __cobalt_byte__(long long x) : v(static_cast<std::uint8_t>(x)) {}
    std::uint8_t v;
};

inline std::ostream& operator<<(std::ostream& os, __cobalt_byte__ b) {
    return os << static_cast<int>(b.v);
}

inline std::istream& operator>>(std::istream& is, __cobalt_byte__& b) {
    int __tmp__;
    if (is >> __tmp__) {
        b.v = static_cast<std::uint8_t>(__tmp__);
    }
    return is;
}

// Type alias so both __byte__ and __cobalt_byte__ refer to the same type
using __byte__ = __cobalt_byte__;

template<typename... Args>
inline void println_c(std::string_view fmt, Args&&... args) {
    std::cout << std::vformat(fmt, std::make_format_args(args...)) << '\n';
}

template<typename... Args>
inline void print_c(std::string_view fmt, Args&&... args) {
    std::cout << std::vformat(fmt, std::make_format_args(args...));
}
#include <cstring>
#include <cstddef>

struct c_string {
    static constexpr size_t MAX_SIZE = 2048;
    char data[MAX_SIZE] = "";
    size_t length = 0;

    static inline bool is_whitespace(char c) {
        return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
    }

    // --- Fast Custom Find (Char) ---
    int find(char target, size_t start_pos = 0) const {
        if (start_pos >= length) return -1;
        
        const char* ptr = data + start_pos;
        const char* end_ptr = data + length;

        while (ptr < end_ptr) {
            if (*ptr == target) return static_cast<int>(ptr - data);
            ptr++;
        }
        return -1;
    }

    // --- Fast Custom Find (Substring/Raw Buffer) ---
    int find(const char* needle, size_t needle_len, size_t start_pos = 0) const {
        if (needle_len == 0) return start_pos <= length ? static_cast<int>(start_pos) : -1;
        if (start_pos + needle_len > length || !needle) return -1;

        const char first = needle[0];
        const char* cur = data + start_pos;
        const char* max_search = data + (length - needle_len);

        while (cur <= max_search) {
            if (*cur == first) {
                size_t i = 1;
                while (i < needle_len && cur[i] == needle[i]) {
                    i++;
                }
                if (i == needle_len) {
                    return static_cast<int>(cur - data);
                }
            }
            cur++;
        }
        return -1;
    }

    // Overloads for c_string and C-strings
    int find(const c_string& needle, size_t start_pos = 0) const {
        return find(needle.data, needle.length, start_pos);
    }

    int find(const char* needle, size_t start_pos = 0) const {
        if (!needle) return -1;
        size_t n_len = 0;
        while (needle[n_len] != '\0') n_len++;
        return find(needle, n_len, start_pos);
    }

    // 1. Default Constructor
    c_string() : length(0) { data[0] = '\0'; }

    // 2. Implicit Literal / C-string Constructor
    c_string(const char* str) {
        assign(str);
    }

    // 3. Sliced Constructor (Length-bounded)
    c_string(const char* str, size_t len) {
        if (str && len > 0) {
            length = (len < MAX_SIZE - 1) ? len : MAX_SIZE - 1;
            std::memcpy(data, str, length);
        } else {
            length = 0;
        }
        data[length] = '\0';
    }

    // 4. Copy Constructor
    c_string(const c_string& other) {
        assign(other.data);
    }
	
	operator std::string() const {
		return std::string(data, length);
	}

    // 5. Assignment Operator (const char*)
    c_string& operator=(const char* str) {
        assign(str);
        return *this;
    }

    // 6. Assignment Operator (c_string)
    c_string& operator=(const c_string& other) {
        if (this != &other) {
            assign(other.data);
        }
        return *this;
    }

    // 7. Concatenation Operator
    c_string operator+(const c_string& other) const {
        c_string result = *this;
        if (result.length + other.length < MAX_SIZE) {
            std::memcpy(result.data + result.length, other.data, other.length + 1);
            result.length += other.length;
        }
        return result;
    }

    // 8. Implicit Conversion to `const char*`
    operator const char*() const { return data; }

    // 9. Array Indexing
    char& operator[](size_t index) { return data[index]; }
    const char& operator[](size_t index) const { return data[index]; }

    // Comparison Operators
    bool operator==(const c_string& other) const {
        return std::strcmp(data, other.data) == 0;
    }
    bool operator==(const char* str) const {
        return str && std::strcmp(data, str) == 0;
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

// Stream Operators
inline std::ostream& operator<<(std::ostream& os, const c_string& str) {
    return os << str.data;
}

inline std::istream& operator>>(std::istream& is, c_string& str) {
    std::string temp;
    if (is >> temp) {
        str = temp.c_str();
    }
    return is;
}

// std::formatter specialization
template <>
struct std::formatter<c_string> : std::formatter<std::string_view> {
    auto format(const c_string& s, std::format_context& ctx) const {
        return std::formatter<std::string_view>::format(std::string_view(s.data, s.length), ctx);
    }
};