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
inline int TryStatus() { return cobalt__try_status__; }int64_t __cobalt_if_end_lasttime = 0;

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
    std::vector<int> i = {9, 0};
    std::unique_ptr<std::vector<int>> o = std::make_unique<std::vector<decltype(8)>>(std::vector<int>{8, 8});
    try {
        for (auto&& on : (*(o))) {
            io::writef(on);
            io::writef("\n");
        }
    }
    catch (...) {}
    try {
        for (auto&& h : i) {
            io::writef(h);
            io::writef("\n");
        }
    }
    catch (...) {}
    return 0;
}

