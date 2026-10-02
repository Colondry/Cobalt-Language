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



int Multiply(int a, int b = a);
int main();

int Multiply(int a, int b) {
    return (a * b);
}

int main() {
    syncw_stdio(false);
    println_c("{}", Multiply(unwrap_val(6)));
    println_c("{}", Multiply(unwrap_val(10), unwrap_val(10)));
    return 0;
}

