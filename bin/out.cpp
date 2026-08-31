#include <iostream>
#include <vector>
#include <cstdint>
#include <stdfloat>

#include <utility>
#include <memory>
#include <type_traits>
inline void syncw_stdio(bool s) {
   std::ios_base::sync_with_stdio(s);
}
inline thread_local int cobalt__try_status__ = 0;
inline int TryStatus() { return cobalt__try_status__; }
#include "W:/Cobalt-Language/bin/lib/base/base.hpp"
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

Time Time;
int main() {
    syncw_stdio(false);
    auto t1 = std::make_unique<std::decay_t<decltype(Time.Now())>>(Time.Now());
    std::unique_ptr<const int> limit = std::make_unique<int>(20000000);
    const std::unique_ptr<int> count = std::make_unique<int>(0);
    for (int i = 2; i < (*(limit)); i++) {
        const std::unique_ptr<bool> is_prime = std::make_unique<bool>(true);
        const std::unique_ptr<int> j = std::make_unique<int>(2);
        while ((((*(j)) * (*(j))) <= i)) {
            std::unique_ptr<const int> rem = std::make_unique<int>((i - ((i / (*(j))) * (*(j)))));
            if (((*(rem)) == 0)) {
                (*(is_prime)) = false;
                break;
            }
            (*j)++;
        }
        if ((*(is_prime))) {
            (*count)++;
        }
    }
    auto t2 = std::make_unique<std::decay_t<decltype(Time.Now())>>(Time.Now());
    println_c("Primes found: {}", count.get());
    println_c("Total times is {}", Time.Elapsed((*(t1)), (*(t2))));
    return 0;
}

