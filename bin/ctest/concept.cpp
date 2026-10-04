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
    return 0;
}

