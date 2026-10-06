#ifndef CSTART_HPP
#define CSTART_HPP

#include "inf.hpp"       // toInt, toDouble, toLong, toLonger, toChar, toStr
#include "cotype.hpp"
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <stdexcept>

namespace csm {

// ============================================================================
// Type traits
// ============================================================================

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

template <typename> inline constexpr bool always_false_v = false;
template <typename To, typename From>
[[nodiscard]] To conv(From&& value) {
    using F = std::remove_cv_t<std::remove_reference_t<From>>;
    if constexpr (std::is_same_v<To, F>) {
        return std::forward<From>(value);
    } else if constexpr (is_unique_ptr_v<F>) {
        if (!value) throw std::runtime_error("csm::conv: null unique_ptr");
        return conv<To>(*value);
    } else if constexpr (is_shared_ptr_v<F>) {
        if (!value) throw std::runtime_error("csm::conv: null shared_ptr");
        return conv<To>(*value);
    } else if constexpr (std::is_same_v<F, std::string> || std::is_same_v<F, const char*> || std::is_same_v<F, char*> || std::is_same_v<F, std::string_view>) {
        if      constexpr (std::is_same_v<To, int>)         return toInt(value);
        else if constexpr (std::is_same_v<To, long>)        return toLong(value);
        else if constexpr (std::is_same_v<To, double>)      return toDouble(value);
        else if constexpr (std::is_same_v<To, long double>) return toLonger(value);
        else if constexpr (std::is_same_v<To, char>)        return toChar(value);
        else if constexpr (std::is_same_v<To, bool>)        return toInt(value) != 0;
        else static_assert(always_false_v<To>,
                           "csm::conv: no conversion from std::string to this type");
    } else if constexpr (std::is_same_v<To, std::string>) {
        return toStr(value);
    } else if constexpr (std::is_same_v<To, bool>) {
        if constexpr (std::is_arithmetic_v<F>) return value != 0;
        else static_assert(always_false_v<To>,
                           "csm::conv: only arithmetic types convert to bool");
    } else if constexpr (std::is_arithmetic_v<To> && std::is_arithmetic_v<F>) {
        return static_cast<To>(value);
    } else {
        static_assert(always_false_v<To>,
                      "csm::conv: no conversion available between these types");
    }
}

template <typename T>
[[nodiscard]] std::unique_ptr<std::remove_cv_t<std::remove_reference_t<T>>>
wrap_unique(T&& value) {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    return std::make_unique<U>(std::forward<T>(value));
}

template <typename T>
[[nodiscard]] std::shared_ptr<std::remove_cv_t<std::remove_reference_t<T>>>
wrap_shared(T&& value) {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    return std::make_shared<U>(std::forward<T>(value));
}

// Raw pointer — non-owning view of an existing value.
template <typename T>
[[nodiscard]] T* as_raw(T& value) noexcept {
    return std::addressof(value);
}

}

#endif