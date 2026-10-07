#ifndef CSTART_HPP
#define CSTART_HPP

#include "inf.hpp"
#include "cotype.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <utility>
#include <stdexcept>

namespace csm {
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

    // Convertion
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
            else static_assert(always_false_v<To>,
                            "csm::conv: no conversion from string to this type");
        }
        // ---- To string ----
        else if constexpr (std::is_same_v<To, std::string>) {
            return toStr(value);
        }
        // ---- To bool ----
        else if constexpr (std::is_same_v<To, bool>) {
            if constexpr (std::is_arithmetic_v<F>) return value != 0;
            else static_assert(always_false_v<To>,
                            "csm::conv: only arithmetic types convert to bool");
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
        }
        else {
            static_assert(always_false_v<To>,
                        "csm::conv: no conversion available between these types");
        }
    }

    // Wrapping helpers
    template <typename T>
    [[nodiscard]] std::unique_ptr<std::decay_t<T>>
    wrap_unique(T&& value) {
        return std::make_unique<std::decay_t<T>>(std::forward<T>(value));
    }

    template <typename T>
    [[nodiscard]] std::shared_ptr<std::decay_t<T>>
    wrap_shared(T&& value) {
        return std::make_shared<std::decay_t<T>>(std::forward<T>(value));
    }

    template <typename T>
    [[nodiscard]] T* as_raw(T& value) noexcept {
        return std::addressof(value);
    }
}

#endif