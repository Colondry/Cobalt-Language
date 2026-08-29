#ifndef FSYS_HPP
#define FSYS_HPP

#include <filesystem>
#include <fstream>
#include <cstdio>
#include "errors.hpp"
#include "inf.hpp"

namespace csm {
    namespace fs = std::filesystem;

    template <typename T>
    inline void crdir(const T& path) {
        fs::create_directories(unwrap_val(path));
    }

    template <typename T>
    bool nwfile(const T& path) {
        decltype(auto) u_path = unwrap_val(path);
        std::FILE* fptr = std::fopen(u_path, "w");
        if (!fptr) return false;
        std::fclose(fptr);
        return true;
    }

    template <typename T1, typename T2>
    inline std::FILE* fopen(const T1& path, const T2& mode) {
        return std::fopen(unwrap_val(path), unwrap_val(mode));
    }

    template <typename T>
    inline void fwrite(std::FILE* file, const T& text) {
        if (file) std::fputs(unwrap_val(text), file);
    }

    inline void fclose(std::FILE* file) {
        if (file) std::fclose(file);
    }
}

#endif