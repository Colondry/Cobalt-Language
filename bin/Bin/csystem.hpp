#ifndef CSYSTEM
#define CSYSTEM

#include <cstdlib>
#include <string>
#include "inf.hpp"

namespace csm {
    inline int WITH_FAILURE = 1;
    inline int WITH_SUCCESS = 0;
    
    template <typename T> inline void exit(const T& code) { std::exit(unwrap_val(code)); }
    inline void abort() { std::abort(); }
    template <typename T> inline void qexit(const T& code) { std::quick_exit(unwrap_val(code)); }
    template <typename T> inline void command(const T& cmd) { std::system(unwrap_val(cmd).c_str()); }
}

#endif