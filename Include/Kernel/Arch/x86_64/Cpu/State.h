#pragma once

#include <LibC/stdint.h>
#include <LibFK/enforce.h>

struct [[gnu::packed]] CpuState {
    LibC::uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    LibC::uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    LibC::uint64_t interrupt_id;
    LibC::uint64_t error_code;
    LibC::uint64_t rip, cs, rflags, rsp, ss;
};
