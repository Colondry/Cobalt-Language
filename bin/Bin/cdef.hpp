#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
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
#include <memory>
#include <cstring>
#include <vector>
#include <charconv>
#include <random>
#include <cctype>
#include <cmath>
#include "cotype.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

// Smart pointer and unique pointer unwraper
template <typename T>
decltype(auto) unwrap_val(T&& val) {
    using Decayed = std::decay_t<T>;
    
    // std::unique_ptr / std::shared_ptr
    if constexpr (requires { val.get(); }) {
        return val ? *val : typename std::remove_cvref_t<decltype(*val)>{};
    } 
    // Raw pointers (excluding string literals / C-strings)
    else if constexpr (std::is_pointer_v<Decayed>) {
        using UncvElement = std::remove_cv_t<std::remove_pointer_t<Decayed>>;
        if constexpr (std::is_same_v<UncvElement, char> || 
                      std::is_same_v<UncvElement, wchar_t> ||
                      std::is_same_v<UncvElement, void>) {
            return std::forward<T>(val); // Keep C-strings and void* as pointers
        } else {
            return val ? *val : std::remove_pointer_t<Decayed>{};
        }
    } 
    else {
        return std::forward<T>(val);
    }
}
// Helper traits to detect std::unique_ptr
template <typename T>
struct is_unique_ptr : std::false_type {};

template <typename T, typename Deleter>
struct is_unique_ptr<std::unique_ptr<T, Deleter>> : std::true_type {};

template <typename T>
inline constexpr bool is_unique_ptr_v = is_unique_ptr<std::decay_t<T>>::value;


#if defined(__STDCPP_FLOAT128_T__)
inline std::ostream& operator<<(std::ostream& os, std::float128_t v) {
    return os << static_cast<long double>(v);
}
#endif

inline char* readln(const char* prompt = "") {
    if (prompt && prompt[0] != '\0') std::cout << prompt;
    // static buffer: returning a local vector's data() would dangle
    static std::string buf;
    std::getline(std::cin, buf);
    return buf.data();
}

inline char* read(const char* prompt = "") {
    if (prompt && prompt[0] != '\0') std::cout << prompt;

    static std::string buf;
    buf.clear();
    std::cin >> buf;
    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        throw std::runtime_error("Scan failed");
    }
    return buf.data();
}

template<typename T1, typename T2>
class frac;

template<typename T1, typename T2>
[[nodiscard]] constexpr frac<T1, T2> simplified(const frac<T1, T2>& f);

template<typename T1, typename T2>
class frac {
public:
    T1 first{};
    T2 second{};

    frac() = default;
    frac(T1 f) : first(unwrap_val(f)), second(1) {}
    constexpr frac(T1 f, T2 s) : first(unwrap_val(f)), second(unwrap_val(s)) {}

    constexpr operator std::pair<T1, T2>() const {
        return {first, second};
    }

    explicit constexpr operator double() const {
        return static_cast<double>(first) / static_cast<double>(second);
    }

    explicit constexpr operator float() const {
        return static_cast<float>(first) / static_cast<float>(second);
    }

    explicit constexpr operator long double() const {
        return static_cast<long double>(first) / static_cast<long double>(second);
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

    constexpr frac& simplify() {
        if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
            if (second == 0) return *this;
            auto g = std::gcd(first, second);
            if (g != 0) {
                first /= g;
                second /= g;
            }
            if constexpr (std::is_signed_v<T2>) {
                if (second < 0) {
                    first = -first;
                    second = -second;
                }
            }
        }
        return *this;
    }

    constexpr frac& operator+=(const frac& other) {
        first = (first * other.second) + (other.first * second);
        second = second * other.second;
        return simplify();
    }

    constexpr frac& operator-=(const frac& other) {
        first = (first * other.second) - (other.first * second);
        second = second * other.second;
        return simplify();
    }

    constexpr frac& operator*=(const frac& other) {
        // Overflow-safe cross-simplification before multiplication
        frac f1 = simplified(frac(first, other.second));
        frac f2 = simplified(frac(other.first, second));
        first = f1.first * f2.first;
        second = f1.second * f2.second;
        return simplify();
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
};

template<typename T1, typename T2>
[[nodiscard]] constexpr frac<T1, T2> simplified(const frac<T1, T2>& f) {
    frac<T1, T2> result = f;
    return result.simplify();
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

// --- std::format support: prints as "first/second" (e.g. 1/3, 3/1) ---
template <typename T1, typename T2>
struct std::formatter<frac<T1, T2>, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}')
            throw std::format_error("frac does not support format specifiers");
        return it;
    }
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

// Single-argument printing
template <typename T>
inline void println_c(T&& val) {
    std::cout << unwrap_val(val) << '\n';
}

template <typename T>
inline void print_c(T&& val) {
    std::cout << unwrap_val(val);
}

inline void println_c(std::string_view fmt) {
    std::cout << fmt << '\n';
}

template <typename... Args>
inline void println_c(std::string_view fmt, Args&&... args) {
    auto print_helper = [&](auto&&... unwrapped_args) {
        std::cout << std::vformat(fmt, std::make_format_args(unwrapped_args...)) << '\n';
    };
    print_helper(unwrap_val(std::forward<Args>(args))...);
}

inline void print_c(std::string_view fmt) {
    std::cout << fmt;
}
template <typename T1, typename T2>
class string {
public:
    static constexpr size_t MAX_SIZE = 5012;
    char data[MAX_SIZE] = "";
    size_t length = 0;

    bool startsWith(const char* prefix) const {
        if (!prefix) return false;
        size_t prefix_len = std::strlen(prefix);
        if (prefix_len > length) return false;
        return std::strncmp(data, prefix, prefix_len) == 0;
    }
    bool endsWith(const char* suffix) const {
        if (!suffix) return false;
        size_t suffix_len = std::strlen(suffix);
        if (suffix_len > length) return false;
        return std::strncmp(data + length - suffix_len, suffix, suffix_len) == 0;
    }
    string substr(size_t start, size_t len) const {
        if (start >= length) return string();
        size_t actual_len = std::min(len, length - start);
        return string(data + start, actual_len);
    }
    string substr(size_t start) const {
        if (start >= length) return string();
        return string(data + start, length - start);
    }
    string toUpper() const {
        string result;
        result.length = length;
        for (size_t i = 0; i < length; ++i) {
            result.data[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(data[i])));
        }
        result.data[result.length] = '\0';
        return result;
    }
    string toLower() const {
        string result;
        result.length = length;
        for (size_t i = 0; i < length; ++i) {
            result.data[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(data[i])));
        }
        result.data[result.length] = '\0';
        return result;
    }
    static inline bool is_whitespace(char c) {
        return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
    }
    // Find character
    size_t find(char target, size_t start_pos = 0) const {
        if (start_pos >= length) return -1;
        
        const char* ptr = data + start_pos;
        const char* end_ptr = data + length;

        while (ptr < end_ptr) {
            if (*ptr == target) return ptr - data;
            ptr++;
        }
        return -1;
    }
    // Fast Custom Find (Substring/Raw Buffer)
    size_t find(const char* needle, size_t needle_len, size_t start_pos = 0) const {
        if (needle_len == 0) return start_pos <= length ? start_pos : -1;
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
                    return cur - data; // Found the substring, return the index
                }
            }
            cur++;
        }
        return -1;
    }

    // Overloads for string
    size_t findf(const string& needle, size_t start_pos = 0) const {
        return find(needle.data, needle.length, start_pos);
    }
    size_t findf(const char* needle, size_t start_pos = 0) const {
        if (!needle) return -1;
        size_t n_len = 0;
        while (needle[n_len] != '\0') n_len++;
        return find(needle, n_len, start_pos);
    }
    // Replace words from the string with another word
    string& replace(const char* target, const char* replacement = "") {
        if (!target || target[0] == '\0') return *this; // Nothing to replace

        size_t target_len = std::strlen(target);
        size_t repl_len = replacement ? std::strlen(replacement) : 0;

        int pos = find(target, target_len, 0);
        if (pos == -1) return *this; // Target not found

        // Calculate new length
        size_t new_length = length - target_len + repl_len;
        if (new_length >= MAX_SIZE) new_length = MAX_SIZE - 1;

        // Move the tail part of the string to make space for replacement
        if (repl_len != target_len) {
            std::memmove(data + pos + repl_len, data + pos + target_len, length - (pos + target_len));
        }

        // Copy the replacement into position
        if (repl_len > 0) {
            std::memcpy(data + pos, replacement, repl_len);
        }

        length = new_length;
        data[length] = '\0'; // Null-terminate
        return *this;
    }string& replace(const string& target, const string& replacement = "")
     {
        return replace(target.data, replacement.data);
    }
    string& replace(T1& target, T2& replacement) {
        decltype(auto) u_target = unwrap_val(target);
        decltype(auto) u_replacement = unwrap_val(replacement);

        if constexpr (std::is_convertible_v<decltype(u_target), const char*> &&
                      std::is_convertible_v<decltype(u_replacement), const char*>) {
            return replace(static_cast<const char*>(u_target), static_cast<const char*>(u_replacement));
        } else {
            return *this; // No-op if types are not convertible to const char*
        }
    }
    // Erase words from the string
    string& erase(const char* target) {
        return replace(target, "");
    }
    string& erase(const string& target) {
        return replace(target.data, "");
    }
    string& erase(T1& target) {
        decltype(auto) u_target = unwrap_val(target);
        if constexpr (std::is_convertible_v<decltype(u_target), const char*>) {
            return erase(static_cast<const char*>(u_target));
        } else {
            return *this; // No-op if type is not convertible to const char*
        }
    }


    string() : length(0) { data[0] = '\0'; } // Default Constructor
    string(const char* str) { assign(str); } // String constructor

    // Sliced Constructor
    string(const char* str, size_t len) {
        if (str && len > 0) {
            length = (len < MAX_SIZE - 1) ? len : MAX_SIZE - 1;
            std::memcpy(data, str, length);
        } else {
            length = 0;
        }
        data[length] = '\0';
    }
    // Copy Constructor
    string(const string& other) {
        assign(other.data);
    }
    // String View Constructor
    string(std::string_view sv) {
        length = std::min(sv.size(), MAX_SIZE - 1);
        std::memcpy(data, sv.data(), length);
        data[length] = '\0';
    }
    // Integer & Numeric Constructors
    string(long long val) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%lld", val);
        assign(buf);
    }
    string(int val) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d", val);
        assign(buf);
    }
    
    // Conversions
    operator std::string() const { return std::string(data, length); }
    operator std::string_view() const { return std::string_view(data, length); }
    operator const char*() const { return data; }

    // Assignment Operators
    string& operator=(const char* str) {
        assign(str);
        return *this;
    }
    string& operator=(const string& other) {
        if (this != &other) {
            assign(other.data);
        }
        return *this;
    }
    string& operator=(std::string_view sv) {
        length = std::min(sv.size(), MAX_SIZE - 1);
        std::memcpy(data, sv.data(), length);
        data[length] = '\0';
        return *this;
    }

    // Concatenation Operator
    string operator+(const string& other) const {
        string result = *this;
        if (result.length + other.length < MAX_SIZE) {
            std::memcpy(result.data + result.length, other.data, other.length + 1);
            result.length += other.length;
        }
        return result;
    }

    // Array Indexing
    char& operator[](size_t index) { return data[index]; }
    const char& operator[](size_t index) const { return data[index]; }

    // Comparison Operators
    bool operator==(const string& other) const { return std::strcmp(data, other.data) == 0; }
    bool operator==(const char* str) const { return str && std::strcmp(data, str) == 0; }

    // Stream Operators
    friend std::ostream& operator<<(std::ostream& os, const string& str) { return os << str.data; }
    friend std::istream& operator>>(std::istream& is, string& str) {
        std::string temp;
        if (is >> temp) {
            str.assign(temp.c_str());
        }
        return is;
    }

private:
    void assign(const char* str) {
        if (!str) {
            data[0] = '\0';
            length = 0;
            return;
        }
        length = std::strlen(str);
        if (length >= MAX_SIZE) length = MAX_SIZE-1;
        std::memcpy(data, str, length);
        data[length] = '\0';
    }
};

// --- std::format support for custom types ---
template <>
struct std::formatter<::string, char> : std::formatter<std::string_view, char> {
    auto format(const ::string& s, std::format_context& ctx) const {
        return std::formatter<std::string_view, char>::format(
            std::string_view(s.data, s.length), ctx);
    }
};
template <>
struct std::formatter<__cobalt_byte__, char> : std::formatter<int, char> {
    auto format(__cobalt_byte__ b, std::format_context& ctx) const {
        return std::formatter<int, char>::format(static_cast<int>(b.v), ctx);
    }
};

// --- Dynamically-typed element: remembers whether it was assigned a char, an integer or a float ---
struct __cobalt_cell__ {
    enum class Kind : std::uint8_t { Int = 0, Char = 1, Float = 2 };
    Kind kind = Kind::Int;
    union {
        long long i = 0;
        char c;
        double d;
    };

    __cobalt_cell__() = default;
    template <typename U> requires std::is_arithmetic_v<U>
    __cobalt_cell__(U v) { *this = v; }

    template <typename U> requires std::is_arithmetic_v<U>
    __cobalt_cell__& operator=(U v) {
        i = 0; // clear all bytes of the union first
        if constexpr (std::is_same_v<U, char>) {            // 'N'  -> character
            kind = Kind::Char; c = v;
        } else if constexpr (std::is_floating_point_v<U>) { // 3.5  -> float
            kind = Kind::Float; d = static_cast<double>(v);
        } else {                                            // 1897, uint8_t, bool... -> number
            kind = Kind::Int; i = static_cast<long long>(v);
        }
        return *this;
    }

    bool is_char()  const { return kind == Kind::Char; }
    bool is_int()   const { return kind == Kind::Int; }
    bool is_float() const { return kind == Kind::Float; }

    // Read as any arithmetic type: m[2].as<int>(), static_cast<double>(m[0])
    template <typename U> requires std::is_arithmetic_v<U>
    U as() const {
        switch (kind) {
            case Kind::Char:  return static_cast<U>(c);
            case Kind::Float: return static_cast<U>(d);
            default:          return static_cast<U>(i);
        }
    }
    template <typename U> requires std::is_arithmetic_v<U>
    explicit operator U() const { return as<U>(); }

    // Comparisons compare values: m[0] == 'N', m[1] == 1897
    template <typename U> requires std::is_arithmetic_v<U>
    bool operator==(U v) const {
        if (kind == Kind::Float || std::is_floating_point_v<U>)
            return as<double>() == static_cast<double>(v);
        return as<long long>() == static_cast<long long>(v);
    }
    bool operator==(const __cobalt_cell__& o) const {
        if (kind == Kind::Float || o.kind == Kind::Float)
            return as<double>() == o.as<double>();
        return as<long long>() == o.as<long long>();
    }
};

inline std::ostream& operator<<(std::ostream& os, const __cobalt_cell__& v) {
    switch (v.kind) {
        case __cobalt_cell__::Kind::Char:  return os << v.c;
        case __cobalt_cell__::Kind::Float: return os << v.d;
        default:                           return os << v.i;
    }
}

template <>
struct std::formatter<__cobalt_cell__, char> : std::formatter<std::string_view, char> {
    auto format(const __cobalt_cell__& v, std::format_context& ctx) const {
        std::string s;
        switch (v.kind) {
            case __cobalt_cell__::Kind::Char:  s = std::string(1, v.c); break;
            case __cobalt_cell__::Kind::Float: s = std::format("{}", v.d); break;
            default:                           s = std::format("{}", v.i); break;
        }
        return std::formatter<std::string_view, char>::format(s, ctx);
    }
};

// The default element type is __cobalt_cell__, so every slot auto-detects char / int / float.
template<typename T = __cobalt_cell__>
class Memory {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Memory<T> uses malloc/realloc, so T must be trivially copyable");
public:
    size_t size = 128; // default to 128 elements

    Memory() : size(128), ptr(static_cast<T*>(std::calloc(128, sizeof(T)))) {
        if (!ptr) throw std::bad_alloc();
    }
    // automatic allocation (zero-filled)
    Memory(size_t s) : size(s), ptr(static_cast<T*>(std::calloc(s, sizeof(T)))) {
        if (!ptr && s != 0) throw std::bad_alloc();
    }
    // automatic free when going out of scope (RAII)
    ~Memory() {
        if (ptr) std::free(ptr);
    }
    // mem2 = std::move(mem1)
    Memory(Memory&& other) noexcept
        : size(std::exchange(other.size, 0)),
          ptr(std::exchange(other.ptr, nullptr)) {}
    Memory& operator=(Memory&& other) noexcept {
        if (this != &other) {
            std::free(ptr);
            ptr = std::exchange(other.ptr, nullptr);
            size = std::exchange(other.size, 0);
        }
        return *this;
    }
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    // mem[n]
    T& operator[](size_t n) {
        if (!ptr || n >= size) throw std::out_of_range("Memory index out of range");
        return ptr[n];
    }
    const T& operator[](size_t n) const {
        if (!ptr || n >= size) throw std::out_of_range("Memory index out of range");
        return ptr[n];
    }
    T* data() { return ptr; }
    const T* data() const { return ptr; }

    // set memory size (in elements) & realloc
    void set_size(size_t s) {
        if (!ptr) throw std::runtime_error("Memory is freed");
        realloc(s);
    }
    // reallocate memory (new elements are zero-filled)
    void realloc(size_t s) {
        if (ptr == nullptr) throw std::runtime_error("Memory is freed");
        T* temp = static_cast<T*>(std::realloc(ptr, s * sizeof(T)));
        if (!temp && s != 0) throw std::runtime_error("realloc failed");
        if (temp && s > size) std::memset(static_cast<void*>(temp + size), 0, (s - size) * sizeof(T));
        ptr = temp; size = s;
    }
    // frees memory
    void free() {
        if (ptr == nullptr) throw std::runtime_error("Memory is already freed");
        std::free(ptr);
        ptr = nullptr;
        size = 0;
    }
private:
    T* ptr = nullptr;
};

namespace csm {
    // Random value in [min, max). rand() or rand(0, 0) gives [0, 1).
    inline long double rand(long double min = 0.0L, long double max = 1.0L) {
        if (!std::isfinite(min) || !std::isfinite(max))
            return std::numeric_limits<long double>::quiet_NaN();

        if (min == 0 && max == 0) max = 1;          // default mode
        if (min > max) std::swap(min, max);
        if (min == max) return min;

        auto engine = []() -> std::mt19937_64& {
            static thread_local std::mt19937_64 gen{[] {
                std::random_device rd;
                std::seed_seq seq{rd(), rd(), rd(), rd()};
                std::mt19937_64 g(seq);
                return g;
            }()};
            return gen;
        }();

        constexpr int bits = std::numeric_limits<long double>::digits;
        long double u = std::generate_canonical<long double, bits>(engine); // [0,1)
        if (u >= 1.0L) u = std::nextafter(1.0L, 0.0L);

        // Lerp form avoids overflow of (max - min) for huge ranges
        long double v = min * (1.0L - u) + max * u;
        if (v >= max) v = std::nextafter(max, min); // keep half-open
        if (v < min)  v = min;
        return v;
    }

    // Smoothed random walk: successive values drift instead of jumping.
    inline long double rand_stable(long double min = 0.0L, long double max = 0.0L,
                                long double smoothing = 0.8L) {
        if (!std::isfinite(min) || !std::isfinite(max))
            return std::numeric_limits<long double>::quiet_NaN();
        if (min == 0 && max == 0) max = 1;    
        if (min > max) std::swap(min, max);
        if (min == max) return min;

        static thread_local bool init = false;
        static thread_local long double lo = 0, hi = 0, previous = 0;

        if (!init || lo != min || hi != max) {
            lo = min; hi = max;
            previous = min / 2 + max / 2;           // overflow-safe midpoint
            init = true;
        }

        smoothing = std::clamp(smoothing, 0.0L, 0.999999L);
        const long double target = rand(min, max);
        previous = previous * smoothing + target * (1.0L - smoothing);
        previous = std::clamp(previous, min, max);  // guard against rounding drift
        return previous;
    }

    // Clear the console screen
    void clear() {
        #ifdef _WIN32 // if Windows
            std::system("cls");
        #else // Other than Windows
            std::system("clear");
        #endif
    }
    // Clear the last \n lines in the console
    void clear(int lines) {
        if (lines <= 0) return;
        #ifdef _WIN32
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE) return;

            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode)) return;

            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        #endif
        #ifdef _WIN32
            for (int i = 0; i < lines; ++i) {
                std::cout << "\r" << std::string(80, ' ') << "\r";
            }
        #else
            for (int i = 0; i < lines; ++i) {
                std::cout << "\033[1A\033[2K"; // Move up and clear line
            }
        #endif
    }

    // Type traits
    template <typename T> struct is_unique_ptr : std::false_type {};
    template <typename T, typename D>
    struct is_unique_ptr<std::unique_ptr<T, D>> : std::true_type {};
    template <typename T>
    inline constexpr bool is_unique_ptr_v = is_unique_ptr<std::remove_cv_t<T>>::value;

    template <typename T> struct is_shared_ptr : std::false_type {};
    template <typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
    template <typename T>
    inline constexpr bool is_shared_ptr_v = is_shared_ptr<std::remove_cv_t<T>>::value;

    template <typename T> struct is_frac : std::false_type {};
    template <typename A, typename B> struct is_frac<frac<A, B>> : std::true_type {};

    template <typename T> struct is_vector : std::false_type {};
    template <typename T, typename A> struct is_vector<std::vector<T, A>> : std::true_type {};

    template <typename> inline constexpr bool always_false_v = false;

    // Convertion Utilities
    template <typename To, typename From>
    [[nodiscard]] To conv(From&& value) {
        using F = std::decay_t<From>;   // ← decay, not remove_cv/remove_ref

        // ---- Same type ----
        if constexpr (std::is_same_v<To, F>) {
            return std::forward<From>(value);
        }
        // ---- Unwrap smart pointers ----
        else if constexpr (is_unique_ptr_v<F>) {
            if (!value) throw std::runtime_error("csm::conv: null unique_ptr");
            return conv<To>(*value);
        }
        else if constexpr (is_shared_ptr_v<F>) {
            if (!value) throw std::runtime_error("csm::conv: null shared_ptr");
            return conv<To>(*value);
        }
        // ---- From string-like ----
        else if constexpr (std::is_same_v<F, std::string> ||
                        std::is_same_v<F, std::string_view> ||
                        std::is_same_v<F, const char*> ||
                        std::is_same_v<F, char*>) {
            if      constexpr (std::is_same_v<To, int>)         return toInt(value);
            else if constexpr (std::is_same_v<To, long>)        return toLong(value);
            else if constexpr (std::is_same_v<To, double>)      return toDouble(value);
            else if constexpr (std::is_same_v<To, long double>) return toLonger(value);
            else if constexpr (std::is_same_v<To, char>)        return toChar(value);
            else if constexpr (std::is_same_v<To, bool>)        return toInt(value) != 0;
            else throw std::runtime_error("csm::conv: unsupported conversion from string-like type");
        }
        // ---- To string ----
        else if constexpr (std::is_same_v<To, std::string>) {
            return toStr(value);
        }
        // ---- To bool ----
        else if constexpr (std::is_same_v<To, bool>) {
            if constexpr (std::is_arithmetic_v<F>) return value != 0;
            else throw std::runtime_error("csm::conv: unsupported conversion to bool");
        }
        // ---- Arithmetic → arithmetic ----
        else if constexpr (std::is_arithmetic_v<To> && std::is_arithmetic_v<F>) {
            return static_cast<To>(value);
        }
        // ---- Arithmetic → frac ----
        else if constexpr (is_frac<To>::value && std::is_arithmetic_v<F>) {
            return To{
                static_cast<typename To::first_type>(value),
                static_cast<typename To::second_type>(1)
            };
        }
        // ---- Anything → vector of one element ----
        else if constexpr (is_vector<To>::value) {
            using Elem = typename To::value_type;
            return To{conv<Elem>(std::forward<From>(value))};
        } else {
            throw std::runtime_error("csm::conv: unsupported conversion");
        }
    }

    // To unique_ptr
    template <typename T>
    [[nodiscard]] std::unique_ptr<std::decay_t<T>>
    asUnique(T&& value) {
        return std::make_unique<std::decay_t<T>>(std::forward<T>(value));
    }
    // To shared_ptr
    template <typename T>
    [[nodiscard]] std::shared_ptr<std::decay_t<T>>
    asSharedPtr(T&& value) {
        return std::make_shared<std::decay_t<T>>(std::forward<T>(value));
    }
    // To raw pointer
    template <typename T>
    [[nodiscard]] T* asRaw(T& value) noexcept {
        return std::addressof(value);
    }

    // Time utilities
    namespace Time {
        [[nodiscard]] inline clock_t Now() { return std::clock(); }
        [[nodiscard]] inline long double Elapsed(clock_t start, clock_t end) {
            return static_cast<long double>(end - start) / CLOCKS_PER_SEC;
        }
    }
}