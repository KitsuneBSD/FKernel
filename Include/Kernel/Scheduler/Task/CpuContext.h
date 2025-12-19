#pragma once 

#include <LibFK/Types/types.h>

#ifdef __x86_64__
struct CpuContext {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
};
#endif

CpuContext GetContextForNewTask(
    void (*entry)(),
    uint64_t stack_top,
    bool is_kernel_task
);
