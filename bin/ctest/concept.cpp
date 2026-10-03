#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
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
int main() {
    syncw_stdio(false);
    std::unique_ptr<frac<int, int>> num = std::make_unique<frac<int, int>>(4, 3);
    std::unique_ptr<frac<int, int>> PI = std::make_unique<frac<int, int>>(22, 7);
    std::unique_ptr<int> r = std::make_unique<int>(16);
    std::unique_ptr<float> v = std::make_unique<float>((((*(num)) * (*(PI))) * std::pow((*(r)), 3)));
    println_c("{}", (*(v)));
    return 0;
}

