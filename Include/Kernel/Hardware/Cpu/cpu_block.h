#pragma once

#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Types/types.h>

// Per-CPU data block accessed via GS segment.
// Offsets MUST match syscall_stub.asm and context_switch.asm.
struct CpuControlBlock {
    uint64_t kernel_stack;         // Offset 0
    uint64_t user_rsp;             // Offset 8
    uint64_t saved_rip;            // Offset 16
    uint64_t saved_rflags;         // Offset 24
    uint64_t cpu_id;               // Offset 32
    struct Task* current_task;     // Offset 40
    uint64_t kernel_cr3;           // Offset 48 — KPTI: full kernel PML4 physical address
    uint64_t user_cr3;             // Offset 56 — KPTI: shadow user PML4 physical address
};

// Per-CPU array — each AP sets MSR_GS_BASE to &g_cpu_blocks[cpu_index].
extern CpuControlBlock g_cpu_blocks[MAX_CPUS];

// Read the current CPU id from GS:32 (valid after init_syscalls() has run).
inline uint64_t get_current_cpu_id() {
    return arch_get_cpu_id();
}

// Return the CpuControlBlock for the currently executing CPU.
inline CpuControlBlock& current_cpu_block() {
    return g_cpu_blocks[get_current_cpu_id()];
}
