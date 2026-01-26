#include "Kernel/Hardware/Cpu/cpu_block.h"
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

extern "C" {
void fork_child_trampoline();
}

extern "C" uint64_t sys_vfork(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) {
    auto *parent = SchedulerManager::the().current();
    if (!parent) return -1;

    fk::algorithms::klog("SYSCALL", "sys_vfork: Task %lu forking...", parent->id);

    // 1. Create child task
    Task *child = new Task();
    if (!child) return -1;

    // 2. Clone metadata
    child->id = SchedulerManager::the().generate_pid();
    child->ppid = parent->id;
    child->name = parent->name;
    child->state = TaskState::Ready;
    child->priority = parent->priority;
    child->is_a_kernel_task = parent->is_a_kernel_task;
    child->cwd = parent->cwd;
    child->vfork_parent_id = parent->id; // Mark as vfork child

    // 3. Clone file descriptors
    for (size_t i = 0; i < parent->file_descriptors.size(); ++i) {
        child->file_descriptors.push_back(parent->file_descriptors[i]);
    }

    // 4. Setup Kernel Stack
    const size_t STACK_SIZE = 16 * 1024;
    void *child_stack_mem = kmalloc(STACK_SIZE);
    child->kernel_stack_top = reinterpret_cast<uint64_t>(child_stack_mem) + STACK_SIZE;
    memcpy(child_stack_mem, reinterpret_cast<void*>(parent->kernel_stack_top - STACK_SIZE), STACK_SIZE);

    // 5. Shared Address Space (vfork semantic)
    child->cr3 = parent->cr3;
    child->is_vfork_sharing_address_space = true;
    child->memory_regions = parent->memory_regions;

    // 6. Setup context
    extern CpuControlBlock g_cpu_block;
    child->user_rsp = g_cpu_block.user_rsp;
    child->saved_rip = g_cpu_block.saved_rip;
    child->saved_rflags = g_cpu_block.saved_rflags;
    child->fs_base = parent->fs_base;
    child->gs_base = parent->gs_base;

    uint64_t *child_context_stack = reinterpret_cast<uint64_t *>(child->kernel_stack_top);
    *(--child_context_stack) = parent->user_rsp;     // top - 8
    *(--child_context_stack) = parent->saved_rflags;  // top - 16
    *(--child_context_stack) = parent->saved_rip;     // top - 24
    
    uint64_t *parent_gprs = reinterpret_cast<uint64_t *>(parent->kernel_stack_top - 24 - 56);
    for (int i = 6; i >= 0; --i) { // RAX, RDI, RSI, RDX, R10, R8, R9
        if (i == 6) // RAX
            *(--child_context_stack) = 0; // Child returns 0
        else
            *(--child_context_stack) = parent_gprs[i];
    }

    // 6.5. Inherit callee-saved registers
    uint64_t rbx, rbp, r12, r13, r14, r15;
    asm volatile("mov %%rbx, %0" : "=r"(rbx));
    asm volatile("mov %%rbp, %0" : "=r"(rbp));
    asm volatile("mov %%r12, %0" : "=r"(r12));
    asm volatile("mov %%r13, %0" : "=r"(r13));
    asm volatile("mov %%r14, %0" : "=r"(r14));
    asm volatile("mov %%r15, %0" : "=r"(r15));

    *(--child_context_stack) = (uint64_t)fork_child_trampoline;
    *(--child_context_stack) = r15;
    *(--child_context_stack) = r14;
    *(--child_context_stack) = r13;
    *(--child_context_stack) = r12;
    *(--child_context_stack) = rbp;
    *(--child_context_stack) = rbx;
    child->stack_pointer = reinterpret_cast<uint64_t>(child_context_stack);

    // 7. Add child to scheduler and BLOCK parent
    SchedulerManager::the().add_task(child);
    
    fk::algorithms::klog("SYSCALL", "sys_vfork: Blocking parent %lu until child %lu execs/exits", parent->id, child->id);
    parent->vfork_waiting = true;
    SchedulerManager::the().block_current();
    SchedulerManager::the().schedule();

    return child->id;
}
