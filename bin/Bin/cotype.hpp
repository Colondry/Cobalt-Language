#ifndef COBALT_CAST_HPP
#define COBALT_CAST_HPP

#include <string>
#include <memory>
#include "inf.hpp"

namespace csm {
    template <typename T>
    int toInt(const T& val) {
        using Unwrapped = std::decay_t<decltype(unwrap_val(val))>;
        
        if constexpr (std::is_arithmetic_v<Unwrapped>) {
            return static_cast<int>(unwrap_val(val));
        } else {
            return std::stoi(unwrap_val(val));
        }
    }

    template <typename T>
    double toDouble(const T& val) {
        using Unwrapped = std::decay_t<decltype(unwrap_val(val))>;
        
        if constexpr (std::is_arithmetic_v<Unwrapped>) {
            return static_cast<int>(unwrap_val(val));
        } else {
            return std::stod(unwrap_val(val));
        }
    }

    template <typename T>
    long long toLong(const T& val) {
        using Unwrapped = std::decay_t<decltype(unwrap_val(val))>;
        
        if constexpr (std::is_arithmetic_v<Unwrapped>) {
            return static_cast<int>(unwrap_val(val));
        } else {
            return std::stoll(unwrap_val(val));
        }
    }

    template <typename T>
    long double toLonger(const T& val) {
        using Unwrapped = std::decay_t<decltype(unwrap_val(val))>;
        
        if constexpr (std::is_arithmetic_v<Unwrapped>) {
            return static_cast<int>(unwrap_val(val));
        } else {
            return std::stold(unwrap_val(val));
        }
    }

    template <typename T>
    inline std::string toStr(const T& val) {
        decltype(auto) v = unwrap_val(val);
        using UnwrappedT = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<UnwrappedT, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<UnwrappedT, char>) {
            return std::string(1, v);
        } else {
            return std::to_string(v);
        }
    }

    template <typename T>
    inline char toChar(const T& val) {
        decltype(auto) v = unwrap_val(val);
        using UnwrappedT = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<UnwrappedT, std::string>) {
            return v.empty() ? '\0' : v[0];
        } else {
            return static_cast<char>(v);
        }
    }

    // --- Type & Conversion Validators ---

    // Returns true if the value is NOT an integer or cannot be parsed as one
    template <typename T>
    inline bool notNum(const T& val) {
        decltype(auto) v = unwrap_val(val);
        using UnwrappedT = std::decay_t<decltype(v)>;

        if constexpr (std::is_integral_v<UnwrappedT> && !std::is_same_v<UnwrappedT, bool> && !std::is_same_v<UnwrappedT, char>) {
            return false;
        } else if constexpr (std::is_convertible_v<UnwrappedT, std::string_view>) {
            std::string_view sv(v);
            if (sv.empty()) return true;
            int result;
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
            return ec != std::errc{} || ptr != sv.data() + sv.size();
        } else {
            return true;
        }
    }

    // Returns true if the value is NOT a floating-point/double number
    template <typename T>
    inline bool notFLoat(const T& val) {
        decltype(auto) v = unwrap_val(val);
        using UnwrappedT = std::decay_t<decltype(v)>;

        if constexpr (std::is_arithmetic_v<UnwrappedT> && !std::is_same_v<UnwrappedT, bool> && !std::is_same_v<UnwrappedT, char>) {
            return false;
        } else if constexpr (std::is_convertible_v<UnwrappedT, std::string_view>) {
            std::string_view sv(v);
            if (sv.empty()) return true;
            double result;
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
            return ec != std::errc{} || ptr != sv.data() + sv.size();
        } else {
            return true;
        }
    }



    // Returns true if the value is NOT a single char
    template <typename T>
    inline bool notChar(const T& val) {
        decltype(auto) v = unwrap_val(val);
        using UnwrappedT = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<UnwrappedT, char>) {
            return false;
        } else if constexpr (std::is_convertible_v<UnwrappedT, std::string_view>) {
            std::string_view sv(v);
            return sv.size() != 1;
        } else {
            return true;
        }
    }

    // Returns true if the value is NOT a string or C-string
    template <typename T>
    inline bool notStr(const T& val) {
        decltype(auto) v = unwrap_val(val);
        using UnwrappedT = std::decay_t<decltype(v)>;

        if constexpr (std::is_convertible_v<UnwrappedT, std::string_view>) {
            return false;
        } else {
            return true;
        }
    }
}

#endif