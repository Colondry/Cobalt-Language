#include "sys.hpp"
#include <string>
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
void enableVirtualTerminal() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#endif


void __System__::Clear() {
    // Check for Windows OS (both 32-bit and 64-bit)
    #if defined(_WIN32) || defined(_WIN64)
        std::system("cls");
        // Check for macOS and Linux
    #elif defined(__APPLE__) || 
          defined(__MACH__) || 
          defined(__linux__) || 
          defined(__gnu_linux__)
        std::system("clear");
    #else
        std::cout << "OS not supported for clearing screen." << std::endl;
    #endif
}
bool __System__::isNative(std::string pf) {
    if (pf != "_WIN32" || pf != "_WIN64"
        || pf != "_GNU_LINUX" || pf != "_LINUX") {
        std::cerr << "OS Type not supported!\n";
        return false;
    }
    if (pf == "_WIN32") {
        #if defined(_WIN32)
            return true;
        #else
            return false;
        #endif
    }
    else if (pf == "_WIN64") {
        #if defined(_WIN64)
                return true;
        #else
            return false;
        #endif
    }
    else if (pf == "_LINUX") {
        #if defined(__linux__)
                return true;
        #else
            return false;
        #endif
    }
    else if (pf == "_GNU_LINUX") {
        #if defined(__gnu_linux__)
                return true;
        #else
            return false;
        #endif
    }
    else {
        std::cerr << "Unexpected error!\n";
        return false;
    }
}
void __System__::clearLines(int lines) {
    if (lines <= 0) return;
#ifdef _WIN32
    enableVirtualTerminal();
#endif

    // Move cursor up X lines
    std::cout << "\x1b[" << lines << "A";

    // Clear from cursor to end of screen
    std::cout << "\x1b[J";

    // Flush the buffer to apply changes immediately
    std::cout << std::flush;
}

#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

    void* System_Malloc(size_t size) {
        return std::malloc(size);
    }

    void* System_Realloc(void* ptr, size_t new_size) {
        return std::realloc(ptr, new_size);
    }

    void System_Free(void* ptr) {
        std::free(ptr);
    }

#ifdef __cplusplus
}
#endif

void* __System__::Malloc(size_t size) {
    return System_Malloc(size);
}
void* __System__::ReAlloc(void* ptr, size_t size) {
    return System_Realloc(ptr, size);
}
void __System__::Free(void* ptr) {
    System_Free(ptr);
}