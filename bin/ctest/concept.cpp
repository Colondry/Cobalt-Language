#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <stdfloat>

#include <utility>
#include <memory>
#include <type_traits>
#include <cnow.hpp>
#include <cstart.hpp>

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
inline int getNum() {
    std::unique_ptr<int> a = std::make_unique<int>(((9 * 9) * 9));
    return ((a * a) * a);
}

inline int getNum2() {
    return (getNum() * getNum());
}

int main() {
    syncw_stdio(false);
    if ((10 > 9))  {
        println_c("ello");
    }
    else if ((18 < 9))  {
        println_c("ello");
    }
    else if ((10 == 9))  {
        println_c("ello");
    }
    else if ((10 <= 9))  {
        println_c("ello");
    }
    else  {
        println_c("done!");
    }
    if ((csm::conv<int>(x) > 8))  {
        println_c("x is more than 8");
    }
    return 0;
}

