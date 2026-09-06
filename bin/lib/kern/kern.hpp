#pragma once

#ifndef KERN
#define KERN

#include <cstdint>
#include <cstddef>

// Hardware layout macro attributes
#define SYS_PACKED __attribute__((packed, aligned(1)))
#define SYS_NAKED  __attribute__((naked))
#define SYS_INLINE __attribute__((always_inline)) inline

namespace kernel {
    namespace cpu {

        // --- Port I/O Primitives ---
        SYS_INLINE uint8_t inb(uint16_t port) {
            uint8_t ret;
            __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        SYS_INLINE void outb(uint16_t port, uint8_t val) {
            __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
        }

        SYS_INLINE uint16_t inw(uint16_t port) {
            uint16_t ret;
            __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        SYS_INLINE void outw(uint16_t port, uint16_t val) {
            __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
        }

        SYS_INLINE uint32_t inl(uint16_t port) {
            uint32_t ret;
            __asm__ __volatile__("inl %1, %0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        SYS_INLINE void outl(uint16_t port, uint32_t val) {
            __asm__ __volatile__("outl %0, %1" : : "a"(val), "Nd"(port));
        }

        // --- CPU State & Execution Flags ---
        SYS_INLINE void cli()   { __asm__ __volatile__("cli" ::: "memory"); }
        SYS_INLINE void sti()   { __asm__ __volatile__("sti" ::: "memory"); }
        SYS_INLINE void hlt()   { __asm__ __volatile__("hlt"); }
        SYS_INLINE void pause() { __asm__ __volatile__("pause" ::: "memory"); }
        SYS_INLINE void nop()   { __asm__ __volatile__("nop"); }

        // --- Control Registers ---
        SYS_INLINE uint64_t read_cr0() {
            uint64_t val;
            __asm__ __volatile__("mov %%cr0, %0" : "=r"(val));
            return val;
        }

        SYS_INLINE void write_cr0(uint64_t val) {
            __asm__ __volatile__("mov %0, %%cr0" : : "r"(val) : "memory");
        }

        SYS_INLINE uint64_t read_cr3() {
            uint64_t val;
            __asm__ __volatile__("mov %%cr3, %0" : "=r"(val));
            return val;
        }

        SYS_INLINE void write_cr3(uint64_t phys_addr) {
            __asm__ __volatile__("mov %0, %%cr3" : : "r"(phys_addr) : "memory");
        }

        SYS_INLINE uint64_t read_cr4() {
            uint64_t val;
            __asm__ __volatile__("mov %%cr4, %0" : "=r"(val));
            return val;
        }

        SYS_INLINE void write_cr4(uint64_t val) {
            __asm__ __volatile__("mov %0, %%cr4" : : "r"(val) : "memory");
        }

        // --- Stack Pointer Registers ---
        SYS_INLINE uint64_t read_rsp() {
            uint64_t rsp;
            __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp));
            return rsp;
        }

        SYS_INLINE uint64_t read_rbp() {
            uint64_t rbp;
            __asm__ __volatile__("mov %%rbp, %0" : "=r"(rbp));
            return rbp;
        }

        // --- Time Stamp Counter & MSR ---
        SYS_INLINE uint64_t rdtsc() {
            uint32_t low, high;
            __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
            return (static_cast<uint64_t>(high) << 32) | low;
        }

        SYS_INLINE uint64_t rdmsr(uint32_t msr) {
            uint32_t low, high;
            __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
            return (static_cast<uint64_t>(high) << 32) | low;
        }

        SYS_INLINE void wrmsr(uint32_t msr, uint64_t val) {
            uint32_t low = static_cast<uint32_t>(val);
            uint32_t high = static_cast<uint32_t>(val >> 32);
            __asm__ __volatile__("wrmsr" : : "a"(low), "d"(high), "c"(msr));
        }

        SYS_INLINE void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
            __asm__ __volatile__(
                "cpuid"
                : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                : "a"(leaf)
            );
        }
    }

    namespace mem {

        template <typename T>
        SYS_INLINE T read_mmio(uintptr_t addr) {
            return *reinterpret_cast<volatile T*>(addr);
        }

        template <typename T>
        SYS_INLINE void write_mmio(uintptr_t addr, T val) {
            *reinterpret_cast<volatile T*>(addr) = val;
        }

        SYS_INLINE void invlpg(void* m) {
            __asm__ __volatile__("invlpg (%0)" : : "b"(m) : "memory");
        }

        // --- Memory Fences ---
        SYS_INLINE void barrier() { __asm__ __volatile__("" ::: "memory"); }
        SYS_INLINE void mfence()  { __asm__ __volatile__("mfence" ::: "memory"); }
        SYS_INLINE void lfence()  { __asm__ __volatile__("lfence" ::: "memory"); }
        SYS_INLINE void sfence()  { __asm__ __volatile__("sfence" ::: "memory"); }

        // --- Fast Zero-Dependency Memory Block Routines ---
        SYS_INLINE void fast_zero(void* dest, size_t count) {
            auto* ptr = static_cast<uint64_t*>(dest);
            size_t qwords = count / 8;
            for (size_t i = 0; i < qwords; ++i) {
                ptr[i] = 0;
            }
            auto* tail = reinterpret_cast<uint8_t*>(ptr + qwords);
            for (size_t i = 0; i < (count % 8); ++i) {
                tail[i] = 0;
            }
        }

        SYS_INLINE void fast_copy(void* dest, const void* src, size_t count) {
            auto* d = static_cast<uint64_t*>(dest);
            const auto* s = static_cast<const uint64_t*>(src);
            size_t qwords = count / 8;
            for (size_t i = 0; i < qwords; ++i) {
                d[i] = s[i];
            }
            auto* d_tail = reinterpret_cast<uint8_t*>(d + qwords);
            const auto* s_tail = reinterpret_cast<const uint8_t*>(s + qwords);
            for (size_t i = 0; i < (count % 8); ++i) {
                d_tail[i] = s_tail[i];
            }
        }
    }

    namespace call {

        SYS_INLINE int64_t raw_syscall(int64_t num, 
                                       int64_t a1 = 0, int64_t a2 = 0, int64_t a3 = 0, 
                                       int64_t a4 = 0, int64_t a5 = 0, int64_t a6 = 0) {
            int64_t ret;
            register int64_t r10 __asm__("r10") = a4;
            register int64_t r8  __asm__("r8")  = a5;
            register int64_t r9  __asm__("r9")  = a6;

            __asm__ __volatile__(
                "syscall"
                : "=a"(ret)
                : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
                : "rcx", "r11", "memory"
            );
            return ret;
        }

        #if defined(_WIN32)
        // Direct Win32 Kernel imports (bypasses CRT / stdio completely)
        extern "C" __declspec(dllimport) void* __stdcall GetStdHandle(unsigned long nStdHandle);
        extern "C" __declspec(dllimport) int __stdcall WriteFile(
            void* hFile, 
            const void* lpBuffer, 
            unsigned long nNumberOfBytesToWrite, 
            unsigned long* lpNumberOfBytesWritten, 
            void* lpOverlapped
        );
        #endif

        SYS_INLINE void write(const char* str, size_t len) {
        #if defined(__linux__)
            raw_syscall(1 /* sys_write */, 1 /* stdout */, reinterpret_cast<int64_t>(str), len);
        #elif defined(_WIN32)
            static void* hStdOut = GetStdHandle((unsigned long)-11); // STD_OUTPUT_HANDLE
            unsigned long written = 0;
            WriteFile(hStdOut, str, static_cast<unsigned long>(len), &written, nullptr);
        #endif
        }
        SYS_INLINE void exit_raw(int code) {
            #if defined(__linux__)
                raw_syscall(60 /* sys_exit */, code);
            #endif
            while (true) { kernel::cpu::hlt(); }
        }
    }

    namespace arch {

        // Interrupt Descriptor Table Entry (64-bit x86_64)
        struct SYS_PACKED IDTEntry64 {
            uint16_t offset_low;      // Offset bits 0..15
            uint16_t selector;        // Kernel code segment selector
            uint8_t  ist;             // Interrupt Stack Table offset (0..7)
            uint8_t  type_attributes; // Type and attributes (Gate type, DPL, Present)
            uint16_t offset_mid;      // Offset bits 16..31
            uint32_t offset_high;     // Offset bits 32..63
            uint32_t zero;            // Reserved
        };

        // Pointer for LIDT Instruction
        struct SYS_PACKED IDTPtr {
            uint16_t limit;
            uint64_t base;
        };

        // Global Descriptor Table Entry (64-bit x86_64)
        struct SYS_PACKED GDTEntry64 {
            uint16_t limit_low;
            uint16_t base_low;
            uint8_t  base_mid;
            uint8_t  access_byte;
            uint8_t  limit_high_flags;
            uint8_t  base_high;
        };

        // Pointer for LGDT Instruction
        struct SYS_PACKED GDTPtr {
            uint16_t limit;
            uint64_t base;
        };

        SYS_INLINE void load_idt(const IDTPtr* idt_ptr) {
            __asm__ __volatile__("lidt %0" : : "m"(*idt_ptr));
        }

        SYS_INLINE void load_gdt(const GDTPtr* gdt_ptr) {
            __asm__ __volatile__("lgdt %0" : : "m"(*gdt_ptr));
        }
    }
}

#endif