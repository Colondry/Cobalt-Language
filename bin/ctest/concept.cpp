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


int Multiply(int a, int b) {
    if ((b == 555000555))  {
        b = a;
    }
    return (a * b);
}

int main() {
    syncw_stdio(false);
    println_c("{}", Multiply(unwrap_val(6)));
    println_c("{}", Multiply(unwrap_val(10), unwrap_val(10)));
    println_c("{}", (*(Mul.x)));
    return 0;
}

class Mul {
public:
    int x = (0);
};
#include <csystem.hpp>
#include <cotype.hpp>
#include <fsys.hpp>
#include <errors.hpp>
#include <runtime.hpp>
#include <inf.hpp>
#include <cstr.hpp>
#include <fstream>
#include <cstdio>
