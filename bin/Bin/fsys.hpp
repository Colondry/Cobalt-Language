#ifndef FSYS_HPP
#define FSYS_HPP

#include <filesystem>
#include <fstream>
#include <cstdio>
#include "errors.hpp"

namespace csm {
    namespace fs = std::filesystem;
    inline void crdir(const fs::path& path) {
        fs::create_directories(path);
    }
    bool nwfile(const char* path) {
        std::FILE* fptr = std::fopen(path, "w");
        if (!fptr) return false;
        std::fclose(fptr);
        return true;
    }
    inline std::FILE* fopen(const char* path, const char* mode = "w") {
        return std::fopen(path, mode);
    }
    inline void fwrite(std::FILE* file, const char* text) {
        if (file && text) std::fputs(text, file);
    }
    inline void fclose(std::FILE* file) {
        std::fclose(file);
    }
}

#endif // FSYS_HPP