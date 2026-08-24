#ifndef RUNTIME
#define RUNTIME

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <cstring>
#include <type_traits>
#include "errors.hpp"

struct CobaltArenaNGET {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
};

namespace csm {
    using addr_t = std::uintptr_t; // Integer representing a raw hardware memory address
    using byte_t = std::uint8_t;  // Raw byte type
    template<typename T>
    inline static addr_t mAddressOf(T& ptr) noexcept {
        return reinterpret_cast<addr_t>(&ptr);
    }
    template<typename T>
    inline static void mwrite(addr_t address, T value) noexcept {
        *reinterpret_cast<T*>(address) = value;
    }
    template<typename T>
    inline static T mread(addr_t address) noexcept {
        return *reinterpret_cast<const T*>(address);
    }
    inline static void mwrbyte(addr_t address, size_t offset, byte_t value) noexcept {
        reinterpret_cast<byte_t*>(address)[offset] = value;
    }
    inline static byte_t mrdbyte(addr_t address, size_t offset) noexcept {
        return reinterpret_cast<const byte_t*>(address)[offset];
    }
    // Usage: mcopy(dest_address, src_address, size_in_bytes);
    inline static void mcopy(addr_t dest, addr_t src, size_t size) noexcept {
        std::memcpy(reinterpret_cast<void*>(dest), reinterpret_cast<const void*>(src), size);
    }
    // Usage: int pc = mcopy(src_address, size_in_bytes);
    static addr_t mcopy(addr_t src, size_t size) noexcept {
        addr_t dest = reinterpret_cast<addr_t>(std::malloc(size));
        if (!dest) csm::RuntimeError("Memory allocation failed in mcopy.");
        std::memcpy(reinterpret_cast<void*>(dest), reinterpret_cast<const void*>(src), size);
        return dest;
    }
    static void malloc(size_t size) {
        void* ptr = std::malloc(size);
        if (!ptr) { fprintf(stderr, "Memory allocation failed for size %zu bytes.\n", size); std::exit(EXIT_FAILURE); }
    }
    inline static void mfree(void* ptr) {
        std::free(ptr);
    }
    inline static void mfill(void* dest, int value, size_t size) {
        std::memset(dest, value, size);
    }
    inline CobaltArenaNGET nfield(size_t size) {
        return CobaltArenaNGET { static_cast<uint8_t*>(std::malloc(size)), size, 0};
    }
    void* falloc(CobaltArenaNGET& field, size_t size) {
        size_t al = (size+7)&~7;
        if (field.offset+al>field.capacity) [[unlikely]] return nullptr;
        void* ptr = &field.buffer[field.offset];
        field.offset += al; return ptr;
    }
    inline void fclear(CobaltArenaNGET& field) { field.offset=0; }
    inline void ffree(CobaltArenaNGET& field) { std::free(field.buffer); field.offset=0; field.capacity=0; }
};

#endif