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

#include "W:/Cobalt-Language/bin/lib/base/base.hpp"
#include "W:/Cobalt-Language/bin/lib/kern/kern.hpp"
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
    auto t1 = std::make_unique<const std::decay_t<decltype(Time.Now())>>(Time.Now());
    for (std::unique_ptr<int64_t> i = std::make_unique<int64_t>(0); *i.get() < 1000000; (*i)++) {
        io::writef(i);
        io::writef("\n");
    }
    std::unique_ptr<const double> done1 = std::make_unique<double>(Time.Elapsed((*(t1)), Time.Now()));
    auto tt1 = std::make_unique<const std::decay_t<decltype(Time.Now())>>(Time.Now());
    for (std::unique_ptr<int64_t> i = std::make_unique<int64_t>(0); *i.get() < 1000000; (*i)++) {
        kernel::call::write(csm::ToChar((*(i))), strlen(unwrap_val(i.get())));
        kernel::call::write("\n", 2);
    }
    std::unique_ptr<const double> done2 = std::make_unique<double>(Time.Elapsed((*(tt1)), Time.Now()));
    println_c("Done with io::writef() in {} seconds.", done1.get());
    println_c("Done with kernel::call::write() in {} seconds.", done2.get());
    std::unique_ptr<double> slow = std::make_unique<double>(0);
    std::unique_ptr<double> fast = std::make_unique<double>(0);
    if (((*(done1)) > (*(done2)))) {
        (*(slow)) = (*(done1));
        (*(fast)) = (*(done2));
    }
    else {
        (*(slow)) = (*(done2));
        (*(fast)) = (*(done1));
    }
    const std::unique_ptr<double> percent = std::make_unique<double>(((((*(slow)) / (*(fast))) - 1) * 100));
    if ((done1 > done2)) {
        println_c("println!() is faster than io::writef() by {}% faster.", percent.get());
    }
    else {
        println_c("io::writef() is faster than println!() by {}% faster.", percent.get());
    }
    return 0;
}

