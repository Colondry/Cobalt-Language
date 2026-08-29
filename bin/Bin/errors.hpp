#ifndef ERR
#define ERR

#include <cstdlib>
#include <iostream>
#include <string>
#include <stdexcept>
#include "inf.hpp"

namespace csm {
    class CobaltException : public std::runtime_error {
    public:
        explicit CobaltException(const std::string& msg) 
            : std::runtime_error(msg) {}
    };

    inline void RuntimeError(const char* msg) { std::cout << msg << "\n"; throw CobaltException(msg); }
    inline void RuntimeException(const char* msg) { std::cout << msg << "\n"; throw CobaltException(msg); }
}

#endif