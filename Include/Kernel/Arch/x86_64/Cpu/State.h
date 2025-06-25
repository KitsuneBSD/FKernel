#pragma once

#include <LibC/stdint.h>

struct CpuState {
    LibC::uint64_t r15;
    LibC::uint64_t r14;
    LibC::uint64_t r13;
    LibC::uint64_t r12;
    LibC::uint64_t r11;
    LibC::uint64_t r10;
    LibC::uint64_t r9;
    LibC::uint64_t r8;
    LibC::uint64_t rsi;
    LibC::uint64_t rdi;
    LibC::uint64_t rbp;
    LibC::uint64_t rdx;
    LibC::uint64_t rcx;
    LibC::uint64_t rbx;
    LibC::uint64_t rax;

    LibC::uint64_t error_code;
    LibC::uint64_t rip;
    LibC::uint64_t cs;
    LibC::uint64_t rflags;
    LibC::uint64_t rsp;
    LibC::uint64_t ss;
} __attribute__((packed));
