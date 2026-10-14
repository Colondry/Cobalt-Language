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
class Math {
public:
    std::unique_ptr<int> x = std::make_unique<int>(0);
    std::unique_ptr<int> y = std::make_unique<int>(0);
    Math() = default;
    Math(int x_ = 0, int y_ = 0)
    {
        (*(this->x)) = x_;
        (*(this->y)) = y_;
    }
    int add() {
        return __cadd__((*(x)), (*(y)));
    }
};
int main() {
    syncw_stdio(false);
    Math m1{};
    Math m2{10, 8};
    Math m3{5};
    println_c("{}", m1.add());
    println_c("{}", m2.add());
    println_c("{}", m3.add());
    return 0;
}

