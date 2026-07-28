#pragma once

#include <LibFK/Types/types.h>

// This structure is used to store per-CPU data, accessed via GS segment.
// It MUST match the offsets used in syscall_stub.asm.
struct CpuControlBlock {
    uint64_t kernel_stack;         // Offset 0
    uint64_t user_rsp;             // Offset 8
    uint64_t saved_rip;            // Offset 16
    uint64_t saved_rflags;         // Offset 24
    uint64_t cpu_id;               // Offset 32
    struct Task* current_task;     // Offset 40
};
