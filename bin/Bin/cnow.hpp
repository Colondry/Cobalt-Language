#ifndef CNOW
#define CNOW

#include <charconv>
#include <cstring>
#include <memory>
#include <string_view>
#include <type_traits>

// Force-inlining macro
#if defined(_MSC_VER)
    #define COBALT_INLINE __forceinline
#else
    #define COBALT_INLINE [[gnu::always_inline]] inline
#endif

namespace io {
    // OS System Write Abstraction
    #if defined(_WIN32)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        COBALT_INLINE void sys_write(const void* src, size_t len) {
            static HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD written;
            WriteFile(hOut, src, static_cast<DWORD>(len), &written, nullptr);
        }
    #else
        #include <unistd.h>
        COBALT_INLINE void sys_write(const void* src, size_t len) {
            ::write(1, src, len);
        }
    #endif

    // 4096-Byte Page-Aligned Buffer & Pointer-Bumping State
    constexpr size_t BUF_SIZE = 131072; // 128 KB
    alignas(4096) inline char buf[BUF_SIZE];
    inline char* ptr = buf;
    inline char* end = buf + BUF_SIZE;

    // 2-Digit Lookup Table (LUT) for Branchless Integer Conversion
    alignas(64) static constexpr char digits_lut[200] = {
        '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
        '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
        '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
        '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
        '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
        '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
        '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
        '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
        '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
        '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
    };

    COBALT_INLINE void flush() {
        size_t len = static_cast<size_t>(ptr - buf);
        if (len > 0) {
            sys_write(buf, len);
            ptr = buf;
        }
    }

    // String Literals & Views
    COBALT_INLINE void writef(std::string_view s) {
        if (ptr + s.size() >= end) flush();
        std::memcpy(ptr, s.data(), s.size());
        ptr += s.size();
    }

    // Single Character
    COBALT_INLINE void writef(char c) {
        if (ptr >= end - 1) flush();
        *ptr++ = c;
    }

    // Floating-point numbers (double / float)
    template <typename T>
    COBALT_INLINE void writef(T val) requires std::is_floating_point_v<T> {
        if (ptr + 64 >= end) flush();
        
        auto [p, ec] = std::to_chars(ptr, end, val);
        if (ec == std::errc()) {
            ptr = p;
        }
    }

    // Fast Integers (LUT algorithm)
    template <typename T>
    COBALT_INLINE void writef(T val) requires std::is_integral_v<T> {
        if (ptr + 32 >= end) flush();

        if (val == 0) {
            *ptr++ = '0';
            return;
        }

        if constexpr (std::is_signed_v<T>) {
            if (val < 0) {
                *ptr++ = '-';
                val = -val;
            }
        }

        char temp[32];
        int p = 32;

        while (val >= 100) {
            const auto idx = (val % 100) * 2;
            val /= 100;
            temp[--p] = digits_lut[idx + 1];
            temp[--p] = digits_lut[idx];
        }

        if (val < 10) {
            temp[--p] = static_cast<char>('0' + val);
        } else {
            const auto idx = val * 2;
            temp[--p] = digits_lut[idx + 1];
            temp[--p] = digits_lut[idx];
        }

        const size_t len = 32 - p;
        std::memcpy(ptr, temp + p, len);
        ptr += len;
    }

    // Unique Pointers (Cobalt Managed Values)
    template <typename T>
    COBALT_INLINE void writef(const std::unique_ptr<T>& ptr_val) {
        if (ptr_val) writef(*ptr_val);
    }

    struct Flusher { ~Flusher() { flush(); } } global_flusher;
}
#endif