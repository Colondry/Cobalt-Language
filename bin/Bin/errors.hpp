#ifndef ERR
#define ERR

#include <cstdlib>
#include <iostream>

namespace csm {
    inline void RuntimeError(const char* msg) {
        std::ios_base::sync_with_stdio(false);
        fprintf(stderr, "Runtime Error: %s\n", msg);
        std::exit(EXIT_FAILURE);
    }
    inline void RuntimeError(const std::string& msg) {
        std::ios_base::sync_with_stdio(false);
        fprintf(stderr, "Runtime Error: %s\n", msg.c_str());
        std::exit(EXIT_FAILURE);
    }
}

#endif