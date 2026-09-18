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



int square(int x);
int sumOfSquares(int a, int b);
int main();

int square(int x) {
    return ((*(x)) * (*(x)));
}

int sumOfSquares(int a, int b) {
    return __cadd__(square(unwrap_val((*(a)))), square(unwrap_val((*(b)))));
}

int main() {
    syncw_stdio(false);
    println_c("{}", sumOfSquares(unwrap_val(6), unwrap_val(8)));
    return 0;
}

