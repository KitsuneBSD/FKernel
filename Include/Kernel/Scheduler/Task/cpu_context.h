#pragma once 

#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
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

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};
#endif

CpuContext GetContextForNewTask(
    void (*entry)(),
    uint64_t stack_top,
    bool is_kernel_task
);

void SaveContext(CpuContext& ctx, const InterruptFrame& frame);
void LoadContext(InterruptFrame& frame, const CpuContext& ctx);