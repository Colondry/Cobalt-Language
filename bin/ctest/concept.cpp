#include <iostream>
#include <vector>
#include <cstdint>
#include <stdfloat>

#include <utility>
#include <memory>
#include <type_traits>
#include <cnow.hpp>

inline void syncw_stdio(bool s) {
   std::ios_base::sync_with_stdio(s);
}
inline thread_local int cobalt__try_status__ = 0;
inline int TryStatus() { return cobalt__try_status__; }

#include <csystem.hpp>
#include <cotype.hpp>
#include <fsys.hpp>
#include <errors.hpp>
#include <runtime.hpp>
#include <inf.hpp>
#include <cstr.hpp>
#include <fstream>
#include <cstdio>



int main();

int main() {
    syncw_stdio(false);
    for (std::unique_ptr<int64_t> i = std::make_unique<int64_t>(0); *i.get() < 7; (*i)++) {
    #pragma omp simd
        println_c("{}", (*(i)));
    }
    std::unique_ptr<int> o = std::make_unique<int>(0);
    for (int __value__ = 0; __value__ < 7; __value__++) {
        println_c("{}", (*(o)));
        (*o)++;
    }
    int8;
    i = 0;
    if (((*(i)) == 0)) [[likely]] {
        println_c("i is 0");
    }
    else if (((*(i)) != 0))  {
        (*i)++;
    }
    else [[unlikely]] {
        (*i)--;
    }
    for (int cobalt_do_repeat = 0; cobalt_do_depeat < 7; ++cobalt_do_repeat) {
        println_c("H");
    }
    println_c("{}", (176 % 98));
    std::unique_ptr<int> x = std::make_unique<int>(8);
    std::unique_ptr<int> xnef = std::make_unique<int>((-std::make_unique<>(*(x))));
    println_c("{} {}", (*(x)), (*(xnef)));
    return 0;
}

