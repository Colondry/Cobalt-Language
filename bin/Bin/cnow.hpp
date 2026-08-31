#include <charconv>
#include <cstring>
#include <memory>
#include <string_view>
#include <type_traits>

#if defined(_WIN32)
    #include <io.h>
    #define sys_write _write
#else
    #include <unistd.h>
    #define sys_write write
#endif

constexpr size_t BUF_SIZE = 65536;
inline char buf[BUF_SIZE];
inline size_t pos = 0;

inline void flush() {
    if (pos > 0) {
        sys_write(1, buf, static_cast<unsigned int>(pos));
        pos = 0;
    }
}

// String Literals & Views
inline void writef(std::string_view s) {
    if (pos + s.size() >= BUF_SIZE) flush();
    std::memcpy(buf + pos, s.data(), s.size());
    pos += s.size();
}

// Char
inline void writef(char c) {
    if (pos + 1 >= BUF_SIZE) flush();
    buf[pos++] = c;
}

// integers
template <typename T>
inline void writef(T val) requires std::is_integral_v<T> {
    if (pos + 32 >= BUF_SIZE) flush();
    auto [p, ec] = std::to_chars(buf + pos, buf + BUF_SIZE, val);
    pos = p - buf;
}

// Unique Pointers
template <typename T>
inline void writef(const std::unique_ptr<T>& ptr) {
    if (ptr) writef(*ptr);
}

struct Flusher { ~Flusher() { flush(); } } global_flusher;