#ifndef FSYS_HPP
#define FSYS_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdio>
#include "errors.hpp"

namespace csm {
    namespace fs = std::filesystem;
    inline void crdir(const fs::path& path) {
        fs::create_directories(path);
    }
    inline bool nwfile(const std::string* path) {
        std::FILE* fptr = std::fopen(path->c_str(), "w");
        if (!fptr) return false;
        std::fclose(fptr);
        return true;
    }
    inline std::FILE* fopen(const std::string* path, const char* mode = "w") {
        return std::fopen(path->c_str(), mode);
    }
    inline std::FILE* fopen(const char* path, const char* mode = "w") {
        return std::fopen(path, mode);
    }
    inline void fwrite(std::FILE* file, const std::string* text) {
        if (file && text) std::fputs(text->c_str(), file);
    }
    inline void fwrite(std::FILE* file, const char* text) {
        if (file && text) std::fputs(text, file);
    }
    inline void fclose(std::FILE* file) {
        if (file) std::fclose(file);
		else csm::RuntimeError("File do not exist or corrupted!");
    }

}

#endif // FSYS_HPP