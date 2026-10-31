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

#include <cdef.hpp>
inline void syncw_stdio(bool s) {
   std::ios_base::sync_with_stdio(s);
}
inline thread_local int cobalt__try_status__ = 0;
inline int TryStatus() { return cobalt__try_status__; }


int main(int argc, char** argv) {
    syncw_stdio(false);
    std::vector<string> args;
    for (int i = 0; i < argc; i++) {
        args.emplace_back(argv[i]);
    }
    std::unique_ptr<int> x = std::make_unique<int>(0);
    for (auto&& arg : args) {
        println_c("{}", arg);
    }
    if (((args.size() > 1) && ((args)[1] == "write")))  {
        println_c("Hello, World!");
    }
    println_c("{}", args.size());
    std::unique_ptr<int[]> num = std::make_unique<int[]>(4);
    num[0] = 10;
    num[1] = 1;
    num[2] = 8;
    num[3] = 7;
    int numbers[4]  = {6, 5, 9, 4};
    println_c("{} {}", (num)[3], (numbers)[0], (*(x)));
    Memory m{512};
    println_c("{}", m.size);
    m.set_size(256);
    println_c("{}", m.size);
    (m)[0] = 'N';
    (m)[1] = 7;
    println_c("{} {}", csm::conv<std::string>((m)[0]), (m)[1]);
    return 0;
    return 0;
}

