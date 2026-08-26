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
#include <cstddef>

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